
#include <osc.h>
using namespace klang::optimised;

static const int FMFlex1OperatorSize {3};
static const int FMFlex1FilterSize {3};

struct FMFlex1 : Synth {

	struct FMFlex1Note : public Note {
		// Define array of operators with base signal OSC
		Operator<OSC> ops [FMFlex1OperatorSize]; 
		// Define array of ADSR for each of the operators
		ADSR ops_adsrs [FMFlex1OperatorSize];
		// ADSR for the final operator result
		ADSR adsr;
		// Filter to apply to the final operator result
		std::unique_ptr<Filter> filter [FMFlex1FilterSize] = {
			std::make_unique<LPF>(),
			std::make_unique<HPF>(),
			std::make_unique<BPF>()
		};
		// Frequency played on key
		param f0;
		
		event on(Pitch pitch, Amplitude velocity) {
		
			f0 = pitch -> Frequency;
			
			// Params to setup OSC oscillator
			const param detune = controls[6] * 0.01f;
			std::array<int, OSCSize>    types {(int)controls[0], (int)controls[1], (int)controls[2]};
			std::array<float, OSCSize>  gains {controls[3], controls[4], controls[5]};
			
			// If selected algorithm is None, then simply output 
			//	OSC with AMP on it; else, setup each operator
			//	according to its ADSR parameters
			if ((int)controls[7] == 3) {
				ops[0].set(f0, detune, types, gains);
				ops_adsrs[0].set(controls[26], 
							     controls[27], 
							     controls[28], 
							     controls[29]);
				ops[0] = ops_adsrs[0];
			} else {
				for (int op = 0; op < FMFlex1OperatorSize; op ++) {
					ops[op].set(f0 * controls[8 + 6*op], detune, types, gains);
					ops_adsrs[op].set(controls[10 + 6*op],
								   	  controls[11 + 6*op],
								      controls[12 + 6*op],
								      controls[13 + 6*op]);
		
					ops[op] = ops_adsrs[op];
				}
			}
			
			
			// Envelope right before output. Not applied if algorithm is None
			adsr(controls[26], controls[27], controls[28], controls[29]);
			
		}

		event off(Amplitude velocity) {
			// Call Envelope::release
			if ((int)controls[7] == 3) { 
				// If algorithm is none, we applied AMP on Op1
				//	 and we wish to call Envelope::release with 
				//	 AMP's R parameter. To do so, I added an extra 
				//	 method to the Operator struct to allow calling 
				//	 its internal env.release()
				ops[0].release(controls[29]);
			} else { 
				adsr.release();
			}
		}
		

		void process() {
			param f = controls[31];
			param Q = controls[32];
			
			// Set each filter to the input frequency and Q values
			for (int flt = 0; flt < FMFlex1FilterSize; flt ++) { filter[flt]->set(f, Q);}
			
			signal mix;
			
			// Performs different operations on OSC output based on selected algorithm
			switch((int)controls[7]) {
				case 0: ((ops[0] * controls[9]) >> (ops[1] * controls[15]) >> (ops[2] * controls[21])) * adsr >> mix; break;
				case 1: ((ops[0] * controls[9]) + (ops[1] * controls[15]) + (ops[2] * controls[21])) * adsr >> mix; break;
				case 2: (((ops[1] * controls[15]) >> (ops[2] * controls[21]) ) + (ops[0] * controls[9])) * adsr >> mix; break;
				case 3: ops[0] >> mix; break;
			}
			
			// Applies different filter based on input
			switch((int)controls[30]){
				case 0: mix >> (*filter[0]) >> out; break;
				case 1: mix >> (*filter[1]) >> out; break;
				case 2: mix >> (*filter[2]) >> out; break;
				case 3: mix >> out;
			}
			
		
			if (adsr.finished())
				stop();
		}
	};

	// Initialise plugin (called once at startup)
	FMFlex1() {
		controls = {
			
			{ "OSC",	Menu("", { 32, 120, 80, 20 }, "Sine", "Saw", "Square", "Triangle", "Function", "Sample"),
						Menu("", { 32, 165, 80, 20 }, "Sine", "Saw", "Square", "Triangle", "Function", "Sample"),
						Menu("", { 32, 210, 80, 20 }, "Sine", "Saw", "Square", "Triangle", "Function", "Sample"),
						Slider("", 0, 1, 1, { 32, 140, 80, 20 } ), 
						Slider("", 0, 1, 1, { 32, 185, 80, 20 } ),
						Slider("", 0, 1, 1, { 32, 230, 80, 20 } ), 
						Dial("Detune", 0, 1, 0, { 55, 75, 40, 40 } )
			},		
			
			Menu("", { 27, 18, 90, 17 }, "ALG1", "ALG2", "ALG3", "None"),	
		
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
			
			{ "Filter", Menu("", { 278, 222, 71, 20 }, "Low-pass", "High-pass", "Band-pass", "None" ),
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