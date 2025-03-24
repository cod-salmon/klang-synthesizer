#include <klang.h>
#include <fstream>          // for file operations
#include <sstream>          // for file operations
#include <filesystem>       // for finding current path
#include <iostream>         // for cerr

using namespace klang::optimised;

static const int OSCSize {3};
static const int OSCOptionSize {10};
static const int OSCFunctionSize {1000};
static const int OSCSampleSize {1000};

//! Uses additive synthesis to build an Oscillator 
//  type out of a set of freq. vs. dB points.
struct OSCSample : public Oscillator {
    private:
        std::pair<float, dB> harmonics [OSCSampleSize];     // array of <float, dB> pairs holding freq. and loudness for each oscillator
        Sine osc [OSCSampleSize];                           // array of Sine oscillators, each one setup according to "harmonics"
        int harmonicSize {};                                // actual number of oscillators to add-up
        float f0 {};                                        // fundamental frequency, assumed to be first frequency on file
    public:
        //! Constructor. Takes an integer that helps identify the file where the (freq., dB) points that define
        //      the sample are stored. Points are read from file and stored in the "harmonics" array.
        OSCSample (const int& o) {
            // Open the text file for the specific Sample
            std::filesystem::path cwd = std::filesystem::current_path();
            std::string filename = cwd.string() + "/../include/oscillators/Sample_" + std::to_string(o) + ".txt";
            std::ifstream file(filename);

            // Check if the file is successfully opened
            if (!file.is_open()) {
                debug.print("OSCSample::OSCSample: Error opening the file ", filename.c_str(), "\n");
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

        //! Implements Oscillator::set(param) by setting each of  
        //  the composing harmonic to the right frequency
        void set (param f) {
            for ( int o = 0; o < harmonicSize; o++ ) {
                osc[o].set(f * harmonics[o].first);
            }
        }
    
        //! Implements Oscillator::process by adding up
        //  the signal from each of the composing harmonics
        void process() {
            signal mix = 0;
            for ( int o = 0; o < harmonicSize; o++ ) {
                mix += osc[o] * harmonics[o].second->Amplitude;
            }
            mix >> out;
        }
};

//! Uses the Envelope struct to build a 
//  signal out of a set of data points
struct OSCFunction : Oscillator {
    private:
        std::vector<Envelope::Point> points = std::vector<Envelope::Point>(
            OSCFunctionSize, Envelope::Point());                                /* Allocate a lot of memory to hopefully 
                                                                                be able to accomodate all points in file */
        int envSize {};                                                         // actual number of data points in the input file
        Envelope env;                                                           // Envelope to initialise with "envSize" "points"
    
    public:
        //! Takes an integer that helps identify the file where the data points that define
        //  the envelope are stored. Points are read from file and stored in a vector.
        //  The number of points defining the envelope gets computed and stored at "envSize".
        OSCFunction (const int& o) {
            // Open the text file for the specific OSCFunction
            std::filesystem::path cwd = std::filesystem::current_path();
            std::string filename = cwd.string() + "/../include/oscillators/Function_" + std::to_string(o) + ".txt";
            std::ifstream file(filename);

            // Check if the file is successfully opened
            if (!file.is_open()) {
                debug.print("OSCFunction::OSCFunction: Error opening file ", filename.c_str(), "\n");
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

        //! Implements Oscillator::set by setting 
        //  the Envelope to loop at frequency f
        void set (param f) {
            //  We expect the input file to define a whole and only one cycle.
            //  That way, when given the frequency f, we can obtain the period
            //  and fit our envelope within that period range to meet 
            //  the frequency constraint.

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

        //! Implements Oscillator::process by
        //  sending Envelope::process() to out
        void process () {
            env >> out;
        }
};

/* TODO: Create a OSCSamples and OSCFunctions structs to 
clear up the Oscillator array in OSCOscillators*/

//! Contains an array of OSCOptionSize different Oscillator types
//  and an int type that helps tracking the one setup Oscillator
struct OSCOscillators : public Oscillator {
    private:
        int type {}; 

        std::unique_ptr<Oscillator> oscs [OSCOptionSize] = {
            std::make_unique<Sine>(),
            std::make_unique<Saw>(),
            std::make_unique<Square>(),
            std::make_unique<Triangle>(),
            std::make_unique<Wavetable>(OSCFunction(1)),
            std::make_unique<Wavetable>(OSCSample(1)),
            std::make_unique<Wavetable>(OSCFunction(2)),
            std::make_unique<Wavetable>(OSCSample(2)),
            std::make_unique<Wavetable>(OSCFunction(3)),
            std::make_unique<Wavetable>(OSCSample(3)),
        }; 

    public:
        //! Sets the oscillator frequency
        void set (const int& o, const int& t, const param& f) { 
            if (t == 4 || t == 5) {
                type = t + 2*o;
            }  else {type = t;}
            oscs[type]->set(f);   
                
        }

        /* //! Need this in order to be able to use OSCOscillators
        //  not only as signal generator but also as 
        void set(relative phase) override { 
            oscs[type]->set(phase);
        } */

        //! Outputs the specific Oscillator::process()
        void process () { (*oscs[type]) >> out; }
};


struct OSC: public Oscillator {
    private:
        std::unique_ptr<OSCOscillators> oscs [OSCSize] = {
            std::make_unique<OSCOscillators>(),
            std::make_unique<OSCOscillators>(),
            std::make_unique<OSCOscillators>()
        };                                                   // the three oscillators
        param gains [OSCSize];                               // the gain for each of the oscillators

    public:
        //! Sets up the oscillators in OSC
        void set(const param& f, 
                const param& d, 
                const std::array<int, OSCSize>& ts, 
                const std::array<float, OSCSize>& gs
                ) {
            // Obtain the type of each oscillator in OSC
            for (int o = 0; o < OSCSize; o++) {
                oscs[o]->set(o, ts[o], f * (1 + (o - 1) * d));
                // Obtain the gain for each oscillator in OSC
                gains[o] = gs[o];
            }
        }

        /* void set(relative phase) override { 
            for (int o = 0; o < OSCSize; o++) {
                oscs[o]->set(phase);
            }
        } */

        //! Outputs the mixed signal of each of the oscillators in OSC
        void process() {
            signal mix;
            for (int o = 0; o < OSCSize; o++) {
                mix += (*oscs[o]) * gains[o];
            }
            mix >> out;
        }
};