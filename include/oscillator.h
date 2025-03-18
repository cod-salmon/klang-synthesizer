#include <klang.h>
#include <fstream>          // for file operations
#include <sstream>          // for file operations
#include <filesystem>       // for finding current path

namespace asynth {
    static const int OSCSize {3};
    static const int OSCFunctionSize {1000};
    static const int OSCSampleSize {100};
}

//! Uses additive synthesis to build an Oscillator 
//  type out of a set of freq. vs. dB points.
struct OSCSample : public Oscillator {
    std::pair<float, dB> harmonics [asynth::OSCSampleSize];     // array of <float, dB> pairs holding freq. and loudness for each oscillator
    Sine osc [asynth::OSCSampleSize];                           // array of Sine oscillators, each one setup according to "harmonics"
    int harmonicSize {};                                        // actual number of oscillators to add-up
    float f0 {};                                                // fundamental frequency, assumed to be first frequency on file
    public:
        //! Constructor. Takes an integer that helps identify the file where the (freq., dB) points that define
        //      the sample are stored. Points are read from file and stored in the "harmonics" array.
        OSCSample (int o) {
            // Open the text file for the specific Sample
            std::filesystem::path cwd = std::filesystem::current_path();
            std::string filename = cwd.string() + "/../include/oscillators/Sample_" + std::to_string(o + 1) + ".txt";
            std::ifstream file(filename);

            // Check if the file is successfully opened
            if (!file.is_open()) {
                debug.print("Error opening the file ", filename.c_str(), "\n");
            }
            
            // Read the file, line by line
            std::string line; int p = 0;                  
            while (std::getline(file, line, '\n')) {
                // Each line holds two floats separated by a whitespace
                //  representing the freq. and the loudness in dB
                std::stringstream ss(line); int count = 0;
                while(getline(ss, line, ' ')){          
                    if (f0 == 0) { // still not setup fundamental frequency
                        f0 = std::stof(line);
                    } else {
                        if (count == 0) {   // we are looking at frequency
                            harmonics[p].first = std::stof(line)/f0;
                        } else {            // we are looking at dB
                            harmonics[p].second = std::stof(line);
                        }
                    }
                    count ++;
                }
                count = 0; // reset count
                p++;
            }

            harmonicSize = p;
            // Close the file
            file.close();        
        }

        void set(param frequency) {
            for ( int o = 0; o < harmonicSize; o++ ) {
                osc[o].set(frequency * harmonics[o].first);
            }
        }
    
        void process() {
            signal mix = 0;
            for ( int o = 0; o < harmonicSize; o++ ) {
                mix += osc[o] * harmonics[o].second->Amplitude;
            }
            mix >> out;
        }
};

struct OSCFunction : public Oscillator {
    // Allocate a lot of memory to hopefully be able to accomodate all points in file
    std::vector<Envelope::Point> points = std::vector<Envelope::Point>(asynth::OSCFunctionSize, Envelope::Point());
    int envSize {}; // actual number of data points in the input file
    Envelope env; // Envelope to initialise with "envSize" "points"
    
    public:
        //! Constructor. Takes an integer that helps identify the file where the data points that define
        //      the envelope are stored. Points are read from file and stored in a vector.
        OSCFunction (int o) {
            // Open the text file for the specific Function
            std::filesystem::path cwd = std::filesystem::current_path();
            std::string filename = cwd.string() + "/../include/oscillators/Function_" + std::to_string(o + 1) + ".txt";
            std::ifstream file(filename);

            // Check if the file is successfully opened
            if (!file.is_open()) {
                debug.print("Error opening the file ", filename.c_str(), "\n");
            }
        
            // Read the file, line by line
            std::string line; int p = 0;                  
            while (std::getline(file, line, '\n')) {
                // Each line holds two floats separated by a whitespace
                //  representing the x and y coordinates
                std::stringstream ss(line);
                float point[2]; int count = 0;
                while(getline(ss, line, ' ')){          
                    point[count++] = std::stof(line);
                }
                points[p] = Envelope::Point(point[0], point[1]);
                count = 0; // reset count
                p++;
            }

            envSize = p; // number of points read
            file.close();
        } 

        //! We expect the input file to define a whole and only one cycle.
        //      That way, when given the frequency f, we can obtain the period
        //      and fit our envelope within that period range to meet 
        //      the frequency constraint.
        void set (param f) {
            param t = 1. / f;

            float scalex = points[envSize - 1].x;
            for (int p = 0; p < envSize; p++) 
            { 
                points[p].x = points[p].x * t / scalex;
                points[p].y = points[p].y;
            }
            // Now properly set the envelope
            env.set(points);
            env.setLoop(0, envSize - 1);
        }

        void process () {
            env >> out;
        }
};

struct OSC: public Oscillator {
    private:
    std::unique_ptr<Oscillator> oscs [asynth::OSCSize]; // the three oscillators
    param gains [asynth::OSCSize];                      // the gain for each of the oscillators

    public:
    //! Sets up the oscillators in OSC
    void set(const param& f, const param& detune, NoteBase<klang::Synth>::Controls& controls) {
        // Obtain the type of each oscillator in OSC
        for (int o = 0; o < asynth::OSCSize; o++) {
			switch((int)controls[o + asynth::OSCSize]){
				case 0: {
                    oscs[o] = std::make_unique<Sine>();
                    break;
                }
				case 1: {
                    oscs[o] = std::make_unique<Saw>();
                    break;
                }
				case 2: {
                    oscs[o] = std::make_unique<Square>();
                    break;
                }
				case 3: {
                    oscs[o] = std::make_unique<Triangle>();
                    break;
                }
				case 4: {
                    oscs[o] = std::make_unique<Wavetable>(OSCFunction(o));
                    break;
                }
                case 5: {
                    oscs[o] = std::make_unique<Wavetable>(OSCSample(o));
                    break;
                }
                default:
                    oscs[o] = std::make_unique<Sine>();
                    break;
			}
            // Obtain the gain for each oscillator in OSC
            gains[o] = controls[o];
            // Set the frequency and detuning for each oscillator in OSC
            oscs[o]->set(f * (1 + (o - 1) * detune));
		}
    }

    //! Outputs the mixed signal of each of the oscillators in OSC
    void process() {
        signal mix;
        for (int o = 0; o < asynth::OSCSize; o++) {
			mix += (*oscs[o]) * gains[o];
		}
        mix >> out;
    }
};

struct MySynthNote : public Note {
    OSC oscs; 	
	ADSR adsr;
    
	event on(Pitch pitch, Amplitude velocity) {
		const param f = pitch -> Frequency;
		const param detune = controls[6] * 0.01f;
        oscs.set(f, detune, controls);

		adsr(controls[7], controls[8], controls[9], controls[10]);
	}

	event off(Amplitude velocity) {
		adsr.release();
	}

	void process() {
        oscs * adsr >> out;
		if (adsr.finished())
			stop(); 
	}
};