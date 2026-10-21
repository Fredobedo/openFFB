/* No hardware access: intercept the Linux evdev boundary and verify the actual
 * upload/play/stop sequence, including updates and calibration cancellation. */
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include "ffbhelper.h"
#include "ffb.h"
#include "config.h"

static int starts[MAX_EFFECTS], stops[MAX_EFFECTS], uploads, level, last_gain, autocenter;
static int fail_upload, reply_count;
static const unsigned char *serial_data;
static int serial_length, serial_offset;
int __wrap_readBytes(unsigned char *buffer, int amount)
{
    if (serial_offset >= serial_length) return -1;
    int count = serial_length - serial_offset;
    if (count > amount) count = amount;
    if (count > 2) count = 2; // fragmented serial input
    memcpy(buffer, serial_data + serial_offset, count);
    serial_offset += count;
    return count;
}

int __wrap_ioctl(int fd, unsigned long request, ...)
{
    (void)fd;
    va_list ap; va_start(ap, request);
    void *arg = va_arg(ap, void *); va_end(ap);
    if (request == EVIOCSFF) {
        struct ff_effect *effect = arg;
        if (fail_upload) { errno = EIO; return -1; }
        ++uploads;
        if (effect->type == FF_CONSTANT) level = effect->u.constant.level;
        return 0;
    }
    errno = EINVAL;
    return -1;
}

ssize_t __wrap_write(int fd, const void *buffer, size_t length)
{
    (void)fd;
    if (length == sizeof(struct input_event)) {
        const struct input_event *e = buffer;
        assert(e->type == EV_FF);
        if (e->code == FF_GAIN) last_gain = e->value;
        else if (e->code == FF_AUTOCENTER) autocenter = e->value;
        else {
            assert(e->code < MAX_EFFECTS);
            if (e->value) ++starts[e->code]; else ++stops[e->code];
        }
    } else {
        assert(length == 4);
        ++reply_count;
    }
    return length;
}

static void bulk(int power)
{
    unsigned char packet[8] = {0x80,0,0,1,power,0,0,0};
    packet[7] = packet[3] ^ packet[4];
    assert(processPacket(packet) == FFB_STATUS_SUCCESS);
}

int main(void)
{
    running = 1;
    device_handle = 123; // intercepted; not a real descriptor
    for (int i=0; i<MAX_EFFECTS; ++i) ffb_effects[i].id = -1;
    ffb_effects[constant_effect_idx] = (struct ff_effect){.id=0,.type=FF_CONSTANT};
    ffb_effects[constant_effect_async_idx] = (struct ff_effect){.id=1,.type=FF_CONSTANT};
    ffb_effects[spring_effect_async_idx] = (struct ff_effect){.id=2,.type=FF_SPRING};
    supportedFeatures = FF_CONSTANT_LOADED | FF_CONSTANT_ASYNC_LOADED | FF_SPRING_LOADED;
    FFBConfig *cfg=getConfig();
    cfg->autoMinForce=0; cfg->minConstant=0; cfg->maxConstant=100; cfg->calibrationConstant=25;
    SegaFFBControllerState=OPENFFB_READY_SUB_CMD;

    FFBSetGlobalGain(50); assert(last_gain==32767);
    FFBSetGlobalGain(1); assert(last_gain==655);
    FFBSetGlobalGain(-1); assert(last_gain==0);
    FFBSetGlobalGain(101); assert(last_gain==65535);

    FFBTriggerConstantEffect(true, 1.0/128, false);
    assert(level==255 && starts[0]==1 && stops[0]==0);
    FFBTriggerConstantEffect(true, -.25, false);
    assert(level==-8191 && starts[0]==1 && stops[0]==0);
    FFBTriggerConstantEffect(true, 1, false);
    assert(level==32767 && starts[0]==1 && stops[0]==0);
    FFBTriggerConstantEffect(true, 0, false);
    assert(level==0 && stops[0]==1);
    cfg->minConstant=19;
    FFBTriggerConstantEffect(true, 0, false); assert(level==0);
    FFBTriggerConstantEffect(true, 1.0/128, false); assert(level==6432);
    cfg->minConstant=0;
    fail_upload=1;
    int before_stop=stops[0];
    FFBTriggerConstantEffect(true, .5, false); assert(stops[0]>before_stop);
    fail_upload=0;
    int before_start=starts[0];
    FFBTriggerConstantEffect(true, .25, false); assert(starts[0]==before_start+1);
    FFBStopAllEffects(); assert(autocenter==0);

    // Pre-READY compensation ends at the actual controller READY notification.
    SegaFFBControllerState=OPENFFB_NOT_READY_SUB_CMD;
    FFBTriggerConstantEffect(true, .01, false); assert(level==8191);
    unsigned char ready[8]={0x99,OPENFFB_READY_SUB_CMD,0,0,0,0,0,OPENFFB_READY_SUB_CMD};
    processPacket(ready);
    FFBTriggerConstantEffect(true, .01, false); assert(level==327);
    FFBStopAllEffects();

    bulk(32); assert(level==8256);
    before_start=starts[0]; int before_upload=uploads;
    bulk(32); assert(starts[0]==before_start && uploads==before_upload);
    unsigned char reset[8]={0x99,OPENFFB_RESET_DEVICE_SUB_CMD,0,0,0,0,0,OPENFFB_RESET_DEVICE_SUB_CMD};
    processPacket(reset); bulk(32); assert(starts[0]==before_start+1);
    bulk(0); assert(level==0);
    bulk(1); assert(level==258);
    bulk(127); assert(level==32767);
    bulk(0);

    unsigned char fragmented[]={0x13,0x55,0x80,0,0,1,32,0,0,33};
    serial_data=fragmented; serial_length=sizeof(fragmented); serial_offset=0;
    assert(readPacket()==FFB_STATUS_SUCCESS && level==8256);
    assert(readPacket()==FFB_STATUS_ERROR_TIMEOUT);
    bulk(0);

    // Cancel a delayed job before it can issue any motor command.
    ThreadParams *params=malloc(sizeof(*params));
    *params=(ThreadParams){0,.25,100,500};
    before_start=starts[1];
    assert(FFBStartCalibration(WorkerSetPosition,params)==0);
    FFBStopAllEffects(); usleep(120000);
    assert(starts[1]==before_start && reply_count==0);
    // Cancel a running job and ensure it cannot restart or claim success.
    params=malloc(sizeof(*params)); *params=(ThreadParams){0,.25,0,500};
    assert(FFBStartCalibration(WorkerSetPosition,params)==0);
    usleep(50000); bulk(0);
    assert(starts[1]==before_start+1 && stops[1]>0 && reply_count==0);
    before_start=starts[1]; usleep(60000); assert(starts[1]==before_start);
    // StopAll must explicitly clear autocenter, independently of gain.
    autocenter=16384; FFBStopAllEffects(); assert(autocenter==0);
    puts("evdev contract, gain, streaming, reset and worker cancellation passed");
    return 0;
}
