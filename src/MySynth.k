
#include <klang.h>
using namespace klang::optimised;

struct MySynth : Synth {

	struct MySynthNote : public Note {
		Sine osc;
		ADSR adsr;

		event on(Pitch pitch, Amplitude velocity) {
			const param f = pitch -> Frequency;
			osc(f, 0);
			adsr(1, 1, 0.5, 1);
		}

		event off(Amplitude velocity) {
			adsr.release();
		}

		void process() {
			param gain = controls[0];
		
			osc* adsr * gain >> out;
			if (adsr.finished())
				stop();
		}
	};

	// Initialise plugin
	MySynth() {
		controls = { 
			Dial("Level", 0, 1, 1)
		};

		notes.add<MySynthNote>(32);
	}
};