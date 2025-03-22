
#include <osc.h>
using namespace klang::optimised;

struct FMFlex1 : Synth {

	struct FMFlex1Note : public Note {
		Operator<OSC> ops[2];
		ADSR adsr;
		param f0;

		event on(Pitch pitch, Amplitude velocity) {
			f0 = pitch -> Frequency;
			const param detune = controls[6] * 0.01f;
			std::array<int, OSCSize>    types {(int)controls[0], (int)controls[1], (int)controls[2]};
			std::array<float, OSCSize>  gains {controls[3], controls[4], controls[5]};

			for (int op = 0; op < 2; op ++) {
				ops[op].set(f0 * controls[11 + 2*op], detune, types, gains);
				ops[op] = { { 0,1 }, { 1, 0.5 }, { 5, 0.125 } };
			}
			adsr(0.0, 0.05, 0.25, 0.5);
			
		}

		event off(Amplitude velocity) {
			adsr.release();
		}

		void process() {
			((ops[0] * controls[12]) >> (ops[1] * controls[14])) * adsr >> out;
			if (adsr.finished())
				stop();
		}
	};

	// Initialise plugin (called once at startup)
	FMFlex1() {
		controls = {
		
			Menu("", { 27, 18, 90, 17 }, "ALG1", "ALG2", "ALG3"),
			
			{ "OSC",	Menu("", { 32, 120, 80, 20 }, "Sine", "Saw", "Square", "Triangle", "Function", "Sample"),
						Menu("", { 32, 165, 80, 20 }, "Sine", "Saw", "Square", "Triangle", "Function", "Sample"),
						Menu("", { 32, 210, 80, 20 }, "Sine", "Saw", "Square", "Triangle", "Function", "Sample"),
						Slider("", 0, 1, 1, { 32, 140, 80, 20 } ), 
						Slider("", 0, 1, 1, { 32, 185, 80, 20 } ),
						Slider("", 0, 1, 1, { 32, 230, 80, 20 } ), 
						Dial("Detune", 0, 1, 0, { 55, 75, 40, 40 } )
			},			
		
			{ "Op1", Dial("f", 0, 2, 0.5, { 132, 55, 30, 30 }), 
					 Dial("Amt", 0, 10, 1, { 165, 55, 30, 30 }),
					 Slider("A", 0, 1, 0.5, { 137, 110, 10, 70 } ), 
					 Slider("D", 0, 1, 0.5, { 152, 110, 10, 70 } ), 
					 Slider("S", 0, 1, 1.0, { 167, 110, 10, 70 } ), 
					 Slider("R", 0, 1, 0.5, { 182, 110, 10, 70 } ),
			},
			{ "Op2", Dial("f", 0, 2, 0.5, { 212, 55, 30, 30 }), 
					 Dial("Amt", 0, 10, 1, { 245, 55, 30, 30 }),
					 Slider("A", 0, 1, 0.5, { 217, 110, 10, 70 } ), 
					 Slider("D", 0, 1, 0.5, { 232, 110, 10, 70 } ), 
					 Slider("S", 0, 1, 1.0, { 247, 110, 10, 70 } ), 
					 Slider("R", 0, 1, 0.5, { 262, 110, 10, 70 } )
			},
			{ "Op3", Dial("f", 0, 2, 0.5, { 292, 55, 30, 30 }), 
					 Dial("Amt", 0, 10, 1, { 325, 55, 30, 30 }),
					 Slider("A", 0, 1, 0.5, { 297, 110, 10, 70 } ), 
					 Slider("D", 0, 1, 0.5, { 312, 110, 10, 70 } ), 
					 Slider("S", 0, 1, 1.0, { 327, 110, 10, 70 } ), 
					 Slider("R", 0, 1, 0.5, { 342, 110, 10, 70 } )
			},
			{ "AMP", Slider("A", 0, 1, 0.5, { 137, 227, 10, 21 } ), 
					 Slider("D", 0, 1, 0.5, { 156, 227, 10, 21 } ), 
					 Slider("S", 0, 1, 1.0, { 175, 227, 10, 21 } ), 
					 Slider("R", 0, 1, 0.5, { 194, 227, 10, 21 } ),
			},
			
			{ "Filter", Menu("", { 278, 222, 71, 20 }, "Low-pass", "High-pass", "Band-pass" ),
						Dial("f", 100, 10000, 10000, { 225, 227, 20, 20 } ),
						Dial("Q", 0.1, 10, 1, { 248, 227, 20, 20 } ),
						},
			
		};
		
		presets = {
			{ "Bass", { 1.000, 0.500, 1.000, 4.296, 2 } },
		};

		notes.add<FMFlex1Note>(32);
	}
};