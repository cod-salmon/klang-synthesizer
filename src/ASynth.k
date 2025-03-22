#include <osc.h>

using namespace klang::optimised;

struct ASynth : Synth {
	// Initialise plugin (called once at startup)
	ASynth() {
		controls = {
			{ "OSC",	Menu("", { 28, 30, 80, 20 }, "Sine", "Saw", "Square", "Triangle", "Function", "Sample"),
						Menu("", { 113, 30, 80, 20 }, "Sine", "Saw", "Square", "Triangle", "Function", "Sample"),
						Menu("", { 198, 30, 80, 20 }, "Sine", "Saw", "Square", "Triangle", "Function", "Sample"),
						Slider("", 0, 1, 1, { 28, 52, 80, 120 } ), 
						Slider("", 0, 1, 1, { 113, 52, 80, 120 } ),
						Slider("", 0, 1, 1, { 198, 52, 80, 120 } ), 
						Dial("Detune", 0, 1, 0, { 300, 50, 35, 30 } ), 
						Slider("A", 0, 1, 0.5, { 290, 100, 10, 70 } ), 
						Slider("D", 0, 1, 0.5, { 305, 100, 10, 70 } ), 
						Slider("S", 0, 1, 1.0, { 320, 100, 10, 70 } ), 
						Slider("R", 0, 1, 0.5, { 335, 100, 10, 70 } ), 
			},				
					
		};

		notes.add<MySynthNote>(32);
	}
};