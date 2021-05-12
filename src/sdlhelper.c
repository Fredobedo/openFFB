    
#include "debug.h"
#include "sdlhelper.h"
#include "config.h"
#include <time.h>
    
#include<stdio.h>
#include<string.h>

int initHaptic(char* name)
{
    int idxDevice=-1;

    /* Initialize all necessary SDL subsystems */
    SDL_Init(SDL_USED_SUBSYSTEMS);

    if (SDL_NumHaptics() > 0) {
		debug(0, "%d Haptic devices detected.\n", SDL_NumHaptics());


		char *end;
		long lnum = strtol(name, &end, 10); 
		int num = (int) lnum;

		if (end != name && SDL_NumHaptics() > num) {
			idxDevice=num;
		}
		else{
			/* Try to find matching device */
			for (int i = 0; i < SDL_NumHaptics(); i++) {
				if (strcmp(GetHapticSimplifiedName(SDL_HapticName(i)), name) == 0){
					idxDevice=i;
					break;
				}
			}
		}

        if (idxDevice ==-1) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to find device matching '%s', aborting.\n",
                name);
            return 0;
        }

        haptic = SDL_HapticOpen(idxDevice);
        if (haptic == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create the haptic device: %s\n",
                SDL_GetError());
            return 0;
        }
        debug(0, "Device found: %s\n", SDL_HapticName(idxDevice));

		// Store supported Effect Mask
		supported = SDL_HapticQuery(haptic);

        // Let's create effects and upload them to the device
        CreateHapticEffects();

        return 1;
    }
    else {
		debug(0, "Error, no haptic device detected.\n");
        return 0;
    }
}

void CreateHapticEffects()
{
	SDL_HapticEffect tempEffect;

    /* --- SDL_HAPTIC_CONSTANT --- */
    if (supported & SDL_HAPTIC_CONSTANT) {
        SDL_memset(&tempEffect, 0, sizeof(SDL_HapticEffect));
        tempEffect.type = SDL_HAPTIC_CONSTANT;
        tempEffect.constant.direction.type = SDL_HAPTIC_CARTESIAN;
        tempEffect.constant.direction.dir[0] = -1; //left => right is +1 as per this 2d array explanation: https://wiki.libsdl.org/SDL_HapticDirection
        tempEffect.constant.length = getConfig()->feedbackLength; // presumably is ms, but is not documented
        tempEffect.constant.delay = 0;
        tempEffect.constant.level = 9999; // this is an sint16 => -32768 to 32767
        effects.effect_constant_id = SDL_HapticNewEffect(haptic, &tempEffect); 
    }
    /* --- SDL_HAPTIC_FRICTION --- */
    if (supported & SDL_HAPTIC_FRICTION) {
    	//tempEffect = SDL_HapticEffect();
        tempEffect.type = SDL_HAPTIC_FRICTION;
        tempEffect.constant.direction.type = SDL_HAPTIC_CARTESIAN;
        tempEffect.condition.delay = 0;
        tempEffect.condition.length = 5000;
        effects.effect_friction_id = SDL_HapticNewEffect(haptic, &tempEffect);
    }
    /* --- SDL_HAPTIC_SINE --- */
    if (supported & SDL_HAPTIC_SINE) {
        //tempEffect = SDL_HapticEffect();
		tempEffect.type = SDL_HAPTIC_SINE;
		tempEffect.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
		tempEffect.periodic.direction.dir[0] = 1;
		tempEffect.constant.direction.dir[1] = 0; //Y Position
		tempEffect.periodic.period = 100;
		tempEffect.periodic.magnitude = 9000;
		tempEffect.periodic.length = 2000;
		tempEffect.periodic.attack_length = 120;
		tempEffect.periodic.fade_length = 120;
        effects.effect_sine_id = SDL_HapticNewEffect(haptic, &tempEffect);
    }
    /* --- SDL_HAPTIC_SPRING --- */
    if (supported & SDL_HAPTIC_SPRING) {
        //tempEffect = SDL_HapticEffect();
        tempEffect.type = SDL_HAPTIC_SPRING;
        tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
        tempEffect.condition.delay = 0;
        tempEffect.condition.length = 5000;
        effects.effect_spring_id = SDL_HapticNewEffect(haptic, &tempEffect);
    }
    /* --- SDL_HAPTIC_INERTIA --- */
    if (supported & SDL_HAPTIC_INERTIA) {    
	    //tempEffect = SDL_HapticEffect();
        tempEffect.type = SDL_HAPTIC_INERTIA;
        tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
        tempEffect.condition.delay = 0;
        tempEffect.condition.length = 5000;
        effects.effect_inertia_id = SDL_HapticNewEffect(haptic, &tempEffect);
    }
    /* --- SDL_HAPTIC_DAMPER --- */
    if (supported & SDL_HAPTIC_DAMPER) {    
        //tempEffect = SDL_HapticEffect();
        tempEffect.type = SDL_HAPTIC_DAMPER;
        tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
        tempEffect.condition.delay = 0;
        tempEffect.condition.length = 5000;
        effects.effect_damper_id = SDL_HapticNewEffect(haptic, &tempEffect);
    }
    /* --- SDL_HAPTIC_TRIANGLE --- */
    if (supported & SDL_HAPTIC_TRIANGLE) {    
        //tempEffect = SDL_HapticEffect();
        tempEffect.type = SDL_HAPTIC_TRIANGLE;
        tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
        tempEffect.condition.delay = 0;
        tempEffect.condition.length = 5000;
        effects.effect_triangle_id = SDL_HapticNewEffect(haptic, &tempEffect);
    }
    /* --- SDL_HAPTIC_RAMP --- */
    if (supported & SDL_HAPTIC_RAMP) {    
        //tempEffect = SDL_HapticEffect();
        tempEffect.type = SDL_HAPTIC_RAMP;
        tempEffect.ramp.direction.type = SDL_HAPTIC_CARTESIAN;
        tempEffect.ramp.delay = 0;
        tempEffect.ramp.length = 5000;
        effects.effect_ramp_id = SDL_HapticNewEffect(haptic, &tempEffect);
    }
    /* --- SDL_HAPTIC_SAWTOOTHUP --- */
    if (supported & SDL_HAPTIC_SAWTOOTHUP) {    
        //tempEffect = SDL_HapticEffect();
        tempEffect.type = SDL_HAPTIC_SAWTOOTHUP;
        tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
        tempEffect.condition.delay = 0;
        tempEffect.condition.length = 5000;
        effects.effect_sawtoothup_id = SDL_HapticNewEffect(haptic, &tempEffect);
    }
    /* --- SDL_HAPTIC_SAWTOOTHDOWN --- */
    if (supported & SDL_HAPTIC_SAWTOOTHDOWN) {    
        //tempEffect = SDL_HapticEffect();
        tempEffect.type = SDL_HAPTIC_SAWTOOTHDOWN;
        tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
        tempEffect.condition.delay = 0;
        tempEffect.condition.length = 5000;
        effects.effect_sawtoothdown_id = SDL_HapticNewEffect(haptic, &tempEffect);    
    }
    /* --- SDL_HAPTIC_LEFTRIGHT --- */
    if (supported & SDL_HAPTIC_LEFTRIGHT) {    
        //tempEffect = SDL_HapticEffect();
        tempEffect.type = SDL_HAPTIC_LEFTRIGHT;
        tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
        tempEffect.condition.delay = 0;
        tempEffect.condition.length = 5000;
        effects.effect_leftright_id = SDL_HapticNewEffect(haptic, &tempEffect);    
    }
	if(SDL_HapticRumbleSupported(haptic) && getConfig()->enableRumble) {
		SDL_HapticRumbleInit(haptic);
	}	
}

void DumpAvailableHaptics()
{
	/* Initialize the force feedbackness */
	if(SDL_USED_SUBSYSTEMS!=SDL_WasInit(SDL_USED_SUBSYSTEMS))
    	SDL_Init(SDL_USED_SUBSYSTEMS);

    debug(0, "Available Haptics:\n");
    debug(0, "------------------\n");
    int nbrOfHaptics = SDL_NumHaptics();
    if (nbrOfHaptics > 0) {
        for (int i = 0; i < SDL_NumHaptics(); i++) 
            debug(0, "Device[%d]: %s\n", i, GetHapticSimplifiedName(SDL_HapticName(i)));
    } 
    else if (nbrOfHaptics==0) 
        debug(0, "-> Warning, no haptic found!\n");   
    else
        debug(0, "-> Warning, %s\n", SDL_GetError()); 
}

/*
 * Cleans up a bit.
 */
void abort_execution(void)
{
    debug(0, "\nAborting program execution.\n");
	if(haptic){
		SDL_HapticStopAll(haptic);
		SDL_HapticClose(haptic);
	}
    SDL_Quit();
}

char* GetHapticSimplifiedName(const char* name)
{
	char* simplifiedName=malloc (sizeof (char) * 128);;
	strcpy(simplifiedName, name);

	for (int i = 0; i < (int)strlen(simplifiedName); i++)
	{
		simplifiedName[i] = tolower(simplifiedName[i]);
		if (simplifiedName[i] == ' ' || simplifiedName[i] == '/' || simplifiedName[i] == '(' || simplifiedName[i] == ')')
			simplifiedName[i] = '-';
	}
	return simplifiedName;
}
/*
 * Displays information about the haptic device.
 */
char* CheckEffect(unsigned int supported, unsigned int check)
{
    if (supported & check)
        return GREEN "-> OK <-" RESET "\n";
    else
        return RED "Not supported" RESET "\n";
}

void dumpSupportedFeatures()
{
    unsigned int supported;

    supported = SDL_HapticQuery(haptic);
    debug(0, "------------------------------------------------------------------\n");
    debug(0, "-- Checking capabilities:\n");
    debug(0, "------------------------------------------------------------------\n");
    debug(0, "   Nbr of programmable effects for this device: %d\n", SDL_HapticNumEffects(haptic));
    debug(0, "   Nbr of effects the device can play at the same time: %d\n", SDL_HapticNumEffectsPlaying(haptic));
    debug(0, "\n");
    debug(0, "     Supported periodic effects:\n");
    debug(0, "      - sine:         %s", CheckEffect(supported, SDL_HAPTIC_SINE));
    debug(0, "      - left/right:   %s", CheckEffect(supported, SDL_HAPTIC_LEFTRIGHT));
    debug(0, "      - triangle:     %s", CheckEffect(supported, SDL_HAPTIC_TRIANGLE));
    debug(0, "      - sawtoothup:   %s", CheckEffect(supported, SDL_HAPTIC_SAWTOOTHUP));
    debug(0, "      - sawtoothdown: %s", CheckEffect(supported, SDL_HAPTIC_SAWTOOTHDOWN));
    debug(0, "\n");
    debug(0, "     Supported constant effect:\n");
    debug(0, "      - constant:     %s", CheckEffect(supported, SDL_HAPTIC_CONSTANT));
    debug(0, "\n");
    debug(0, "     Supported condition effects:\n");
    debug(0, "      - spring:       %s", CheckEffect(supported, SDL_HAPTIC_SPRING));
    debug(0, "      - damper:       %s", CheckEffect(supported, SDL_HAPTIC_DAMPER));
    debug(0, "      - inertia:      %s", CheckEffect(supported, SDL_HAPTIC_INERTIA));
    debug(0, "      - friction:     %s", CheckEffect(supported, SDL_HAPTIC_FRICTION));
    debug(0, "\n");
    debug(0, "     Supported ramp effect:\n");
    debug(0, "      - ramp:         %s", CheckEffect(supported, SDL_HAPTIC_RAMP));
    debug(0, "\n");
    debug(0, "     Supported custom effect:\n");
    debug(0, "      - custom:       %s", CheckEffect(supported, SDL_HAPTIC_CUSTOM));
    debug(0, "\n");
    debug(0, "     Supported global features:\n");
    debug(0, "      - gain:        %s", CheckEffect(supported, SDL_HAPTIC_GAIN));
    debug(0, "      - autocenter:  %s", CheckEffect(supported, SDL_HAPTIC_AUTOCENTER));
    debug(0, "      - status:      %s", CheckEffect(supported, SDL_HAPTIC_STATUS));
    debug(0, "      - pause:       %s", CheckEffect(supported, SDL_HAPTIC_PAUSE));
    debug(0, "\n");
	debug(0, "     Rumble Supported:\n");
    debug(0, "      - rumble:      %s", CheckEffect(SDL_HapticRumbleSupported(haptic), 1));	
}

void stopEffect(int effect)
{
    if (haptic)
        SDL_HapticStopEffect(haptic, effect);
}

void stopAllEffects()
{
	if(haptic)
		SDL_HapticStopAll(haptic);
}

void TriggerConstantEffect(int direction, double strength)
{
	if (supported & SDL_HAPTIC_CONSTANT) {
		SDL_HapticEffect tempEffect;
		SDL_memset(&tempEffect, 0, sizeof(SDL_HapticEffect));
		tempEffect.type = SDL_HAPTIC_CONSTANT;
		tempEffect.constant.direction.type = SDL_HAPTIC_CARTESIAN;
		tempEffect.constant.direction.dir[0] = direction;
		tempEffect.constant.length = getConfig()->feedbackLength;
		tempEffect.constant.delay = 0;

		int confMinForce = getConfig()->minForce;
		int confMaxForce = getConfig()->maxForce;

		/*
		if (AlternativeFFB == 1)
		{
			if (direction == -1)
			{
				confMinForce = configAlternativeMinForceLeft;
				confMaxForce = configAlternativeMaxForceLeft;
			}
			else
			{
				confMinForce = configAlternativeMinForceRight;
				confMaxForce = configAlternativeMaxForceRight;
			}
		}
		if (PowerMode == 1)
		{
			strength = pow(strength, 0.5);
		}
	*/


		short MinForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0);
		short MaxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = MaxForce - MinForce;
		short level = (short)(strength * range + MinForce);

		if (range > 0 && level < 0)
		{
			level = 32767;
		}
		else if (range < 0 && level > 0)
		{
			level = -32767;
		}

		tempEffect.constant.level = level;

		SDL_HapticUpdateEffect(haptic, effects.effect_constant_id, &tempEffect);
		SDL_HapticRunEffect(haptic, effects.effect_constant_id, 1);
	}
	else if(getConfig()->enableRumble)
		SDL_HapticRumblePlay(haptic, strength, getConfig()->feedbackLength);
}

void TriggerInertiaEffect(double strength)
{
	if (supported & SDL_HAPTIC_INERTIA) { 
		SDL_HapticEffect tempEffect;
		SDL_memset(&tempEffect, 0, sizeof(SDL_HapticEffect));
		tempEffect.type = SDL_HAPTIC_INERTIA;
		tempEffect.condition.type = SDL_HAPTIC_INERTIA;
		tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
		tempEffect.condition.delay = 0;
		tempEffect.condition.length =  getConfig()->feedbackLength;
		tempEffect.condition.direction.dir[0] = 1;
		tempEffect.condition.direction.dir[1] = 1; //Y Position
		short minForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short coeff = (short)(strength * range + minForce);
		if (coeff < 0)
		{
			coeff = 32767;
		}

		tempEffect.condition.left_coeff[0] = (short)(coeff);
		tempEffect.condition.right_coeff[0] = (short)(coeff);
		tempEffect.condition.left_sat[0] = (short)(coeff) * 10;
		tempEffect.condition.right_sat[0] = (short)(coeff) * 10;
		tempEffect.condition.center[0] = 0;

		SDL_HapticUpdateEffect(haptic, effects.effect_inertia_id, &tempEffect);
		SDL_HapticRunEffect(haptic, effects.effect_inertia_id, 1);
	}
}

void TriggerTriangleEffect(double strength)
{
    if (supported & SDL_HAPTIC_TRIANGLE) {  	
		int direction = 1;
		if (strength < -0.001) {
			strength *= -1;
			direction = -1;
		}

		SDL_HapticEffect tempEffect;
		SDL_memset(&tempEffect, 0, sizeof(SDL_HapticEffect));
		tempEffect.type = SDL_HAPTIC_TRIANGLE;
		tempEffect.condition.type = SDL_HAPTIC_TRIANGLE;
		tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
		tempEffect.condition.direction.dir[0] = direction;
		tempEffect.condition.direction.dir[1] = 0; //Y Position
		tempEffect.periodic.period = 500;

		int confMinForce = getConfig()->minForce;
		int confMaxForce = getConfig()->maxForce;
		/*
		if ( AlternativeFFB == 1)
		{
			if (direction == -1)
			{
				confMinForce = configAlternativeMinForceLeft;
				confMaxForce = configAlternativeMaxForceLeft;
			}
			else
			{
				confMinForce = configAlternativeMinForceRight;
				confMaxForce = configAlternativeMaxForceRight;
			}
		}
		*/

		short minForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short power = (short)(strength * range + minForce);
		if (range > 0 && power < 0)
		{
			power = 32767;
		}
		else if (range < 0 && power > 0)
		{
			power = -32767;
		}
		tempEffect.periodic.magnitude = power;
		tempEffect.periodic.length = getConfig()->feedbackLength;
		tempEffect.periodic.attack_length = 1000;
		tempEffect.periodic.fade_length = 1000;

		SDL_HapticUpdateEffect(haptic, effects.effect_triangle_id, &tempEffect);
		SDL_HapticRunEffect(haptic, effects.effect_triangle_id, 1);
	}
}

void TriggerDamperEffect(double strength)
{
    if (supported & SDL_HAPTIC_DAMPER) { 	
		SDL_HapticEffect tempEffect;
		SDL_memset(&tempEffect, 0, sizeof(SDL_HapticEffect));
		tempEffect.type = SDL_HAPTIC_DAMPER;
		tempEffect.condition.type = SDL_HAPTIC_DAMPER;
		tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
		tempEffect.condition.delay = 0;
		tempEffect.condition.length = getConfig()->feedbackLength;
		tempEffect.condition.direction.dir[0] = 1; // not used
		tempEffect.condition.direction.dir[1] = 0; //Y Position
		short minForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short coeff = (short)(strength * range + minForce);

		if (coeff < 0)
		{
			coeff = 32767;
		}

		tempEffect.condition.left_coeff[0] = (short)(coeff);
		tempEffect.condition.right_coeff[0] = (short)(coeff);
		tempEffect.condition.left_sat[0] = (uint16_t)(coeff) * 2;
		tempEffect.condition.right_sat[0] = (uint16_t)(coeff) * 2;

		SDL_HapticUpdateEffect(haptic, effects.effect_damper_id, &tempEffect);
		SDL_HapticRunEffect(haptic, effects.effect_damper_id, 1);
	}
}

void TriggerRampEffect(double start, double end)
{
    if (supported & SDL_HAPTIC_RAMP) { 	
		SDL_HapticEffect tempEffect;
		SDL_memset(&tempEffect, 0, sizeof(SDL_HapticEffect));
		tempEffect.type = SDL_HAPTIC_RAMP;
		tempEffect.ramp.type = SDL_HAPTIC_RAMP;
		tempEffect.ramp.direction.type = SDL_HAPTIC_CARTESIAN;
		tempEffect.ramp.length = getConfig()->feedbackLength;
		short minForce = (short)(start > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short start1 = (short)(start * range + minForce);
		if (start1 < 0)
		{
			start1 = 32767;
		}
		short minForce2 = (short)(end > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce2 = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range2 = maxForce - minForce;
		short start2 = (short)(end * range + minForce);
		if (start2 < 0)
		{
			start2 = 32767;
		}
		tempEffect.ramp.delay = 0;
		tempEffect.ramp.start = start1;
		tempEffect.ramp.end = -start2;

		SDL_HapticUpdateEffect(haptic, effects.effect_ramp_id, &tempEffect);
		SDL_HapticRunEffect(haptic, effects.effect_ramp_id, 1);
	}
}

void TriggerSawToothUpEffect(double strength)
{
    if (supported & SDL_HAPTIC_SAWTOOTHUP) {  	
		SDL_HapticEffect tempEffect;
		SDL_memset(&tempEffect, 0, sizeof(SDL_HapticEffect));
		tempEffect.type = SDL_HAPTIC_SAWTOOTHUP;
		tempEffect.condition.type = SDL_HAPTIC_SAWTOOTHUP;
		tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
		tempEffect.periodic.period = 500;
		short minForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short power = (short)(strength * range + minForce);
		if (power < 0)
		{
			power = 32767;
		}
		tempEffect.periodic.magnitude = power;
		tempEffect.periodic.length = getConfig()->feedbackLength;
		tempEffect.periodic.attack_length = 1000;
		tempEffect.periodic.fade_length = 1000;

		SDL_HapticUpdateEffect(haptic, effects.effect_sawtoothup_id, &tempEffect);
		SDL_HapticRunEffect(haptic, effects.effect_sawtoothup_id, 1);
	}
}

void TriggerSawToothDownEffect(double strength)
{
	if (supported & SDL_HAPTIC_SAWTOOTHDOWN) {
		SDL_HapticEffect tempEffect;
		SDL_memset(&tempEffect, 0, sizeof(SDL_HapticEffect));
		tempEffect.type = SDL_HAPTIC_SAWTOOTHDOWN;
		tempEffect.condition.type = SDL_HAPTIC_SAWTOOTHDOWN;
		tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
		tempEffect.periodic.period = 500;
		short minForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short power = (short)(strength * range + minForce);
		if (power < 0)
		{
			power = 32767;
		}
		tempEffect.periodic.magnitude = power;
		tempEffect.periodic.length = getConfig()->feedbackLength;
		tempEffect.periodic.attack_length = 1000;
		tempEffect.periodic.fade_length = 1000;

		SDL_HapticUpdateEffect(haptic, effects.effect_sawtoothdown_id, &tempEffect);
		SDL_HapticRunEffect(haptic, effects.effect_sawtoothdown_id, 1);
	}
}

void TriggerFrictionEffect(double strength)
{
	TriggerFrictionEffectWithDefaultOption(strength, false);
}

void TriggerFrictionEffectWithDefaultOption(double strength, bool isDefault)
{
	if (supported & SDL_HAPTIC_FRICTION) {
		SDL_HapticEffect tempEffect;
		SDL_memset(&tempEffect, 0, sizeof(SDL_HapticEffect));
		tempEffect.type = SDL_HAPTIC_FRICTION;
		tempEffect.condition.type = SDL_HAPTIC_FRICTION;
		tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
		tempEffect.condition.delay = 0;
		tempEffect.condition.length = isDefault ? SDL_HAPTIC_INFINITY : getConfig()->feedbackLength;
		tempEffect.condition.left_sat[0] = 0xFFFF;
		tempEffect.condition.right_sat[0] = 0xFFFF;

		short minForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short coeff = (short)(strength * range + minForce);
		if (coeff < 0)
		{
			coeff = 32767;
		}

		tempEffect.condition.left_coeff[0] = (short)(coeff);
		tempEffect.condition.right_coeff[0] = (short)(coeff);
		SDL_HapticUpdateEffect(haptic, effects.effect_friction_id, &tempEffect);
		SDL_HapticRunEffect(haptic, effects.effect_friction_id, 1);
	}
	else if(getConfig()->enableRumble)
		SDL_HapticRumblePlay(haptic, strength, getConfig()->feedbackLength);
}

void TriggerSineEffect(uint16_t period, uint16_t fadePeriod, double strength)
{
    /*
    if (supported & SDL_HAPTIC_SINE) {	
		std::chrono::milliseconds now = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
		long long elapsedTime = (std::chrono::duration_cast<std::chrono::milliseconds>(now - timeOfLastSineEffect)).count();

		int direction = 1;
		if (strength < -0.001) {
			strength *= -1;
			direction = -1;
		}

		// if no strength, we do nothing
		if (strength <= 0.001) {
			return;
		}

		// we ignore the new effect until the last one is completed, unless the new one is significantly stronger
		if (elapsedTime < lastSineEffectPeriod && strength < (lastSineEffectStrength * 1.5)) {
			return;
		}

		SDL_HapticEffect tempEffect;
		SDL_memset(&tempEffect, 0, sizeof(SDL_HapticEffect));

		tempEffect.type = SDL_HAPTIC_SINE;
		tempEffect.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
		tempEffect.periodic.direction.dir[0] = direction;
		tempEffect.constant.direction.dir[1] = 0; //Y Position
		tempEffect.periodic.period = period;

		int confMinForce = configMinForce;
		int confMaxForce = configMaxForce;
		if (AlternativeFFB == 1)
		{
			if (direction == -1)
			{
				confMinForce = configAlternativeMinForceLeft;
				confMaxForce = configAlternativeMaxForceLeft;
			}
			else
			{
				confMinForce = configAlternativeMinForceRight;
				confMaxForce = configAlternativeMaxForceRight;
			}
		}
		short minForce = (short)(strength > 0.001 ? ( getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short magnitude = (short)(strength * range + minForce);
		if (range > 0 && magnitude < 0)
		{
			magnitude = 32767;
		}
		else if (range < 0 && magnitude > 0)
		{
			magnitude = -32767;
		}

		tempEffect.periodic.magnitude = (short)(magnitude);
		tempEffect.periodic.length = period;
		tempEffect.periodic.attack_length = fadePeriod;
		tempEffect.periodic.fade_length = fadePeriod;

		SDL_HapticUpdateEffect(haptic, effects.effect_sine_id, &tempEffect);
		SDL_HapticRunEffect(haptic, effects.effect_sine_id, 1);

		timeOfLastSineEffect = now;
		lastSineEffectStrength = strength;
		lastSineEffectPeriod = period;
	}
    */
}

void TriggerSpringEffect(double strength)
{
	TriggerSpringEffectWithDefaultOption(strength, false);
}

void TriggerSpringEffectWithDefaultOption(double strength, bool isDefault)
{
	if (supported & SDL_HAPTIC_SPRING) {
		SDL_HapticEffect tempEffect;
		SDL_memset(&tempEffect, 0, sizeof(SDL_HapticEffect));
		tempEffect.type = SDL_HAPTIC_SPRING;
		tempEffect.condition.type = SDL_HAPTIC_SPRING;
		tempEffect.condition.direction.type = SDL_HAPTIC_CARTESIAN;
		tempEffect.condition.delay = 0;
		tempEffect.condition.length = isDefault ? SDL_HAPTIC_INFINITY : getConfig()->feedbackLength;
		tempEffect.condition.direction.dir[0] = 1;
		tempEffect.constant.direction.dir[1] = 0; //Y Position

		short minForce = (short)(strength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
		short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
		short range = maxForce - minForce;
		short coeff = (short)(strength * range + minForce);
		if (coeff < 0)
		{
			coeff = 32767;
		}

		tempEffect.condition.left_coeff[0] = (short)(coeff);
		tempEffect.condition.right_coeff[0] = (short)(coeff);
		tempEffect.condition.left_sat[0] = (short)(coeff * 2.0);  //Needed for Logitech G920 wheel
		tempEffect.condition.right_sat[0] = (short)(coeff * 2.0); //Needed for Logitech G920 wheel
		tempEffect.condition.center[0] = 0;

		SDL_HapticUpdateEffect(haptic, effects.effect_spring_id, &tempEffect);
		SDL_HapticRunEffect(haptic, effects.effect_spring_id, 1);
	}
}

void TriggerLeftRightEffect(double smallstrength, double largestrength)
{
	if (supported & SDL_HAPTIC_LEFTRIGHT) {
		if (getConfig()->enableRumble == 1)
		{
			if (getConfig()->reverseRumble == 0)
			{
				SDL_HapticEffect tempEffect;
				tempEffect.type = SDL_HAPTIC_LEFTRIGHT;
				tempEffect.leftright.length = getConfig()->feedbackLength;
				short minForce = (short)(smallstrength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
				short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
				short range = maxForce - minForce;
				tempEffect.leftright.small_magnitude = (short)(smallstrength * range + minForce);
				short minForce1 = (short)(largestrength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
				short maxForce1 = (short)(getConfig()->maxForce / 100.0 * 32767.0);
				short range1 = maxForce1 - minForce1;
				tempEffect.leftright.large_magnitude = (short)(largestrength * range1 + minForce1);
				SDL_HapticUpdateEffect(haptic, effects.effect_leftright_id, &tempEffect);
				SDL_HapticRunEffect(haptic, effects.effect_leftright_id, 1);
			}
			else if (getConfig()->reverseRumble == 1)
			{
				SDL_HapticEffect tempEffect;
				tempEffect.type = SDL_HAPTIC_LEFTRIGHT;
				tempEffect.leftright.length = getConfig()->feedbackLength;
				short minForce = (short)(largestrength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
				short maxForce = (short)(getConfig()->maxForce / 100.0 * 32767.0);
				short range = maxForce - minForce;
				tempEffect.leftright.small_magnitude = (short)(largestrength * range + minForce);
				short minForce1 = (short)(smallstrength > 0.001 ? (getConfig()->minForce / 100.0 * 32767.0) : 0); // strength is a double so we do an epsilon check of 0.001 instead of > 0.
				short maxForce1 = (short)(getConfig()->maxForce / 100.0 * 32767.0);
				short range1 = maxForce1 - minForce1;
				tempEffect.leftright.large_magnitude = (short)(smallstrength * range1 + minForce1);
				SDL_HapticUpdateEffect(haptic, effects.effect_leftright_id, &tempEffect);
				SDL_HapticRunEffect(haptic, effects.effect_leftright_id, 1);
			}
		}
	}
}

void TriggerAutoCenterEffect(double strength)
{

}

void CenterWheel()
{ 

}

void TriggerEffect(unsigned int effect, double strength)
{
    switch(effect)
    {
        case SDL_HAPTIC_SINE:
            TriggerSineEffect(100, 100, strength);
            break;
        case SDL_HAPTIC_LEFTRIGHT:
            TriggerLeftRightEffect(0,strength);
            TriggerLeftRightEffect(1,strength);
            break;
        case SDL_HAPTIC_TRIANGLE:
            TriggerTriangleEffect(strength);
            break;
        case SDL_HAPTIC_SAWTOOTHUP:
            TriggerSawToothUpEffect(strength);
            break;                        
        case SDL_HAPTIC_SAWTOOTHDOWN:
            TriggerSawToothDownEffect(strength);
            break;
        case SDL_HAPTIC_CONSTANT:
            TriggerConstantEffect(0, strength);
            break;
        case SDL_HAPTIC_SPRING:
            TriggerSpringEffect(strength);
            break;
        case SDL_HAPTIC_DAMPER:
            TriggerDamperEffect(strength);
            break;  
        case SDL_HAPTIC_INERTIA:
            TriggerInertiaEffect(strength);
            break;
        case SDL_HAPTIC_FRICTION:
            TriggerFrictionEffect(strength);
            break;
        case SDL_HAPTIC_RAMP:
            TriggerRampEffect(0.01, 0.1);
            break;
        case SDL_HAPTIC_AUTOCENTER:
            TriggerAutoCenterEffect(strength);
            break;              
    }
}
