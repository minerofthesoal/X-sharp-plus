~~ Neural Network Example in X#
~~ Creates a simple feedforward neural network, trains on XOR, shows predictions

summon Math from "stdlib/math"

~~ Neuron entity: stores weights, bias, and output
entity Neuron {
    morph weights: arsenal[spark]
    morph bias: spark
    morph output: spark = 0.0
    morph delta: spark = 0.0

    forge conjure(num_inputs: blade) {
        self.weights = []
        self.bias = Math.randomRange(-1.0, 1.0)
        cycle (morph i: blade = 0; i < num_inputs; i++) {
            self.weights.push(Math.randomRange(-1.0, 1.0))
        }
    }

    forge activate(inputs: arsenal[spark]) -> spark {
        morph sum: spark = self.bias
        cycle (morph i: blade = 0; i < self.weights.length(); i++) {
            sum = sum + self.weights[i] * inputs[i]
        }
        ~~ Sigmoid activation
        self.output = 1.0 / (1.0 + Math.exp(-sum))
        unleash self.output
    }
}

~~ Layer entity: collection of neurons
entity Layer {
    morph neurons: arsenal[Neuron]
    morph num_inputs: blade

    forge conjure(num_neurons: blade, num_inputs: blade) {
        self.num_inputs = num_inputs
        self.neurons = []
        cycle (morph i: blade = 0; i < num_neurons; i++) {
            self.neurons.push(conjure Neuron(num_inputs))
        }
    }

    forge forward(inputs: arsenal[spark]) -> arsenal[spark] {
        morph outputs: arsenal[spark] = []
        cycle (morph i: blade = 0; i < self.neurons.length(); i++) {
            outputs.push(self.neurons[i].activate(inputs))
        }
        unleash outputs
    }

    forge get_outputs() -> arsenal[spark] {
        morph outputs: arsenal[spark] = []
        cycle (morph i: blade = 0; i < self.neurons.length(); i++) {
            outputs.push(self.neurons[i].output)
        }
        unleash outputs
    }
}

~~ NeuralNetwork entity
entity NeuralNetwork {
    morph layers: arsenal[Layer]
    morph learning_rate: spark

    forge conjure(topology: arsenal[blade], lr: spark) {
        self.learning_rate = lr
        self.layers = []

        ~~ Create layers: topology = [input_size, hidden_size, ..., output_size]
        cycle (morph i: blade = 1; i < topology.length(); i++) {
            self.layers.push(conjure Layer(topology[i], topology[i - 1]))
        }
    }

    forge predict(inputs: arsenal[spark]) -> arsenal[spark] {
        morph current: arsenal[spark] = inputs
        cycle (morph i: blade = 0; i < self.layers.length(); i++) {
            current = self.layers[i].forward(current)
        }
        unleash current
    }

    forge train(inputs: arsenal[spark], targets: arsenal[spark]) -> spark {
        ~~ Forward pass
        morph output: arsenal[spark] = self.predict(inputs)

        ~~ Calculate output layer deltas
        morph output_layer: Layer = self.layers[self.layers.length() - 1]
        morph total_error: spark = 0.0
        cycle (morph i: blade = 0; i < output_layer.neurons.length(); i++) {
            morph o: spark = output_layer.neurons[i].output
            morph t: spark = targets[i]
            morph error: spark = t - o
            total_error = total_error + error * error
            ~~ Sigmoid derivative: o * (1 - o)
            output_layer.neurons[i].delta = error * o * (1.0 - o)
        }

        ~~ Backpropagate through hidden layers
        morph l: blade = self.layers.length() - 2
        while (l >= 0) {
            morph current_layer: Layer = self.layers[l]
            morph next_layer: Layer = self.layers[l + 1]

            cycle (morph i: blade = 0; i < current_layer.neurons.length(); i++) {
                morph error_sum: spark = 0.0
                cycle (morph j: blade = 0; j < next_layer.neurons.length(); j++) {
                    error_sum = error_sum + next_layer.neurons[j].delta * next_layer.neurons[j].weights[i]
                }
                morph o: spark = current_layer.neurons[i].output
                current_layer.neurons[i].delta = error_sum * o * (1.0 - o)
            }
            l = l - 1
        }

        ~~ Update weights
        cycle (morph l_idx: blade = 0; l_idx < self.layers.length(); l_idx++) {
            morph layer: Layer = self.layers[l_idx]
            morph prev_outputs: arsenal[spark]

            oracle (l_idx == 0) {
                prev_outputs = inputs
            } otherwise {
                prev_outputs = self.layers[l_idx - 1].get_outputs()
            }

            cycle (morph i: blade = 0; i < layer.neurons.length(); i++) {
                morph neuron: Neuron = layer.neurons[i]
                cycle (morph j: blade = 0; j < neuron.weights.length(); j++) {
                    neuron.weights[j] = neuron.weights[j] + self.learning_rate * neuron.delta * prev_outputs[j]
                }
                neuron.bias = neuron.bias + self.learning_rate * neuron.delta
            }
        }

        unleash total_error
    }
}

~~ Main entry point
quest() {
    engrave("=== X# Neural Network - XOR Problem ===")
    engrave("")

    ~~ Seed random for reproducibility
    Math.seedRandom(42)

    ~~ Create network: 2 inputs -> 4 hidden -> 1 output
    morph topology: arsenal[blade] = [2, 4, 1]
    morph nn: NeuralNetwork = conjure NeuralNetwork(topology, 2.0)

    ~~ XOR training data
    morph train_inputs: arsenal[arsenal[spark]] = [
        [0.0, 0.0],
        [0.0, 1.0],
        [1.0, 0.0],
        [1.0, 1.0]
    ]
    morph train_targets: arsenal[arsenal[spark]] = [
        [0.0],
        [1.0],
        [1.0],
        [0.0]
    ]

    ~~ Train the network
    morph epochs: blade = 10000
    engrave("Training for #{epochs} epochs...")
    engrave("")

    cycle (morph epoch: blade = 0; epoch < epochs; epoch++) {
        morph total_error: spark = 0.0
        cycle (morph i: blade = 0; i < 4; i++) {
            total_error = total_error + nn.train(train_inputs[i], train_targets[i])
        }

        ~~ Print progress every 2000 epochs
        oracle (epoch % 2000 == 0) {
            engrave("Epoch #{epoch}: error = #{total_error}")
        }
    }

    ~~ Show final predictions
    engrave("")
    engrave("=== Final Predictions ===")
    cycle (morph i: blade = 0; i < 4; i++) {
        morph result: arsenal[spark] = nn.predict(train_inputs[i])
        morph input_a: spark = train_inputs[i][0]
        morph input_b: spark = train_inputs[i][1]
        morph predicted: spark = result[0]
        morph expected: spark = train_targets[i][0]
        morph rounded: blade = (predicted + 0.5) as blade

        engrave("  #{input_a} XOR #{input_b} = #{predicted} (rounded: #{rounded}, expected: #{expected})")
    }

    engrave("")
    engrave("Training complete!")
}
