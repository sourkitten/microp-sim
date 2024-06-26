#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <random>  // Add the missing header
#include <chrono>  // Add the missing header
#include <sstream> 

enum class ElementTypes { NOT, AND };

struct Element {
    ElementTypes type;
    std::vector<int> inputs;
    int output;
};

std::vector<int> SignalsTable;

void process(const Element& element) {
    if (element.type == ElementTypes::AND) {
        SignalsTable[element.output] = SignalsTable[element.inputs[0]] & SignalsTable[element.inputs[1]];
    } else if (element.type == ElementTypes::NOT) {
        SignalsTable[element.output] = !SignalsTable[element.inputs[0]];
    }
}

void simulate(const std::vector<Element>& ElementsTable) {
    /* This uses a naive approach of processes the elements,
    until no output signals are changed. */
    bool changesOccurred = true;

    // Iterate until no more changes occur
    while (changesOccurred) {
        changesOccurred = false;

        for (const auto& element : ElementsTable) {
            int previousOutput = SignalsTable[element.output];

            // Process the element
            process(element);

            // Check if the output changed
            if (previousOutput != SignalsTable[element.output]) {
                changesOccurred = true;
            }
        }
    }
}


void randomInputs(double input1val, double input2val, double input3val) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::bernoulli_distribution dist1(input1val);
    std::bernoulli_distribution dist2(input2val);
    std::bernoulli_distribution dist3(input3val);

    SignalsTable[0] = dist1(gen) ? 1 : 0;
    SignalsTable[1] = dist2(gen) ? 1 : 0;
    SignalsTable[2] = dist3(gen) ? 1 : 0;
}

// This stoobid thing, returns segfault on empty lines. Sooo pls no empty lines UwU
void readCircuitFromFile(const std::string& circuitFile, std::vector<Element>& ElementsTable, std::unordered_set<int>& inputSignals) {
    // Open circuit file
    std::ifstream file(circuitFile);
    if (!file.is_open()) {
        std::cout << "Unable to open circuit file\n";
        exit(1);
    }

    // Jesse, we gotta track processed outputs!!!
    std::string line;
    bool foundTopInputs = false;
    std::unordered_set<int> processedOutputs;

    while (std::getline(file, line)) {
        if (line.find("top_inputs") != std::string::npos) {
            // Found top_inputs, process the inputs
            foundTopInputs = true;
            size_t pos = line.find("top_inputs");
            line.erase(0, pos + 10); // This is history erasure!!!!!!!!
            std::istringstream iss(line);
            std::string input;
            while (iss >> input) {
                if (input != "top_inputs" && !input.empty()) {
                    inputSignals.insert(input[0] - 'a');
                }
            }
        } else {
            // Process the Element
            std::istringstream iss(line);
            std::string type, output, input1, input2;
            iss >> type >> output >> input1 >> input2;

            // Parse Element type
            Element newElement;
            if (type == "AND") {
                newElement.type = ElementTypes::AND;
            } else if (type == "NOT") {
                newElement.type = ElementTypes::NOT;
            }

            // Parse Element IO
            newElement.output = output[0] - 'a';
            if (!input1.empty()) {
                newElement.inputs.push_back(input1[0] - 'a');
            }
            if (!input2.empty()) {
                newElement.inputs.push_back(input2[0] - 'a');
            }

            // Add element to the ElementsTable
            ElementsTable.push_back(newElement);
        }
    }

    // Add the output to processedOutputs set after processing all elements
    for (const Element& element : ElementsTable) {
        processedOutputs.insert(element.output);
    }

    // BEGONE
    file.close();

    // If 'top_inputs' was not found, automatically detect inputs, through parsing the processed elements
    if (!foundTopInputs) {
        for (const Element& element : ElementsTable) {
            for (int input : element.inputs) {
                // Check if the input is not in the processedOutputs list
                if (processedOutputs.find(input) == processedOutputs.end()) {
                    inputSignals.insert(input);
                }
            }
        }
    }
}


std::vector<double> calculateSwitchingActivity(const std::vector<Element>& ElementsTable, int iterations, std::vector<double> inputVals) {
    
    std::vector<double> avg_switches(ElementsTable.size(), 0);

    for (int i = 0; i < iterations; ++i) {
        randomInputs(inputVals[0], inputVals[1], inputVals[2]); // randomize the inputs

        // Save the previous state
        int previous_state[ElementsTable.size()];
        for (size_t j = 0; j < ElementsTable.size(); ++j) {
            previous_state[j] = SignalsTable[ElementsTable[j].output];
        }

        simulate(ElementsTable);

        // Compare the states for changed signals
        for (size_t j = 0; j < ElementsTable.size(); ++j) {
            if (previous_state[j] != SignalsTable[ElementsTable[j].output]) {
                ++avg_switches[j];
            }
        }
    }

    // Calculate the switching activities
    for (auto& switches : avg_switches) {
        switches /= iterations;
    }

    return avg_switches;
}

void readInputValues(const std::string& valuesFile, std::vector<std::vector<double>>& inputValues) {
    std::ifstream file(valuesFile);
    if (file.is_open()) {
        double val1, val2, val3;
        while (file >> val1 >> val2 >> val3) {
            inputValues.push_back({val1, val2, val3});
        }
        file.close();
    } else {
        std::cout << "Unable to open testing values file\n";
        exit(1);
    }
}


void testbench(const std::string& circuitFile, const std::string& valuesFile, int iterations) {
    SignalsTable = std::vector<int>(6, 0); // Assuming 6 signals: a, b, c, d, e, f
    std::vector<Element> ElementsTable;
    std::unordered_set<int> inputSignals;

    // Parse circuit file
    readCircuitFromFile(circuitFile, ElementsTable, inputSignals);

    // Create signal vector that matches the number the number of unique signals found
    std::cout << "Inputs declared in '" << circuitFile << "':\n";
    std::vector<int> inputs(inputSignals.begin(), inputSignals.end());

    // Read input values from file
    std::vector<std::vector<double>> inputValues;
    readInputValues(valuesFile, inputValues);

    // Print detected inputs
    bool first = true;
    for (auto it = inputs.rbegin(); it != inputs.rend(); ++it) {
        if (!first) {
            std::cout << ", ";
        }
        std::cout << (char)('a' + *it);
        first = false;
    }
    std::cout << "\n\n";

    // Print the truth table 
    std::cout << "Truth table verification:\n";
    std::vector<std::pair<std::vector<int>, std::vector<int>>> truthTableResults;
    for (int i = 0; i < (1 << inputSignals.size()); ++i) {
        // Set the SignalsTable based on the current input combination
        for (int j = 0; j < inputSignals.size(); ++j) {
            SignalsTable[inputs[j]] = (i >> j) & 1;
        }

        // Simulate the circuit
        simulate(ElementsTable);

        // Store the input values and corresponding output in the vector
        std::vector<int> inputValues;
        for (const auto& elem : inputs) {
            inputValues.push_back(SignalsTable[elem]);
        }
        std::vector<int> outputValues;
        for (const auto& element : ElementsTable) {
            outputValues.push_back(SignalsTable[element.output]);
        }

        // Use a temporary variable for std::pair initialization
        truthTableResults.push_back(std::make_pair(inputValues, outputValues));

        // Print the inputs
        for (size_t j = inputs.size(); j > 0; --j) {
            std::cout << (char)('a' + inputs[j - 1]) << "=" << inputValues[j - 1];
            if (j > 1) {
                std::cout << ", ";
            }
        }
        // Print the outputs
        std::cout << " => ";
        for (const auto& element : ElementsTable) {
            std::cout << (char)('a' + element.output) << "=" << SignalsTable[element.output];
            if (&element != &ElementsTable.back()) {
                std::cout << ", ";
            }
        }
        std::cout << "\n";
    }
    std::cout << "\n";

    // Calculate and print average switching activity of outputs
    std::cout << "Average switching activity for outputs\n";
    for (const auto& inputVals : inputValues) {
        std::vector<double> avg_switch_activity = calculateSwitchingActivity(ElementsTable, iterations, inputVals);
        std::cout << "with inputs (a=" << inputVals[0] << ", b=" << inputVals[1] << ", c=" << inputVals[2] << "):\n";

        // Print output labels and corresponding activities
        for (size_t i = 0; i < avg_switch_activity.size(); ++i) {
            std::cout << (char)('d' + i) << ": " << avg_switch_activity[i] << '\n';
        }
        std::cout << "\n";
    }
}

int main(int argc, char *argv[]) {
    // Check if sufficient arguments are given
    if (argc < 3 or argc > 4){
        std::cout << "Usage: \"" << argv[0] << " circuit.txt values.txt [number of iterations]\"\n";
        exit(1);
    }

    // Retrieve number of iterations, if provided as argument
    int iterations = 1000; // Just in case, ya know
    if (argc == 4) {
        iterations = std::atoi(argv[3]); // Inserted value
    }

    // Run ze code
    testbench(argv[1], argv[2], iterations);

    return 0;
}
