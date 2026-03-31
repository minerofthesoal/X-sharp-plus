/*
 * X# Standard Library - AI Module
 * =================================
 * Neural network + matrix operations: 17 functions.
 * Implements a real feedforward neural network with backpropagation.
 */

#ifndef XS_AI_LIB_H
#define XS_AI_LIB_H

#include "../../src/runtime/runtime.h"

/* Neural network */
XsValue xs_ai_createNeuralNet(int argc, XsValue* args);
XsValue xs_ai_addLayer(int argc, XsValue* args);
XsValue xs_ai_train(int argc, XsValue* args);
XsValue xs_ai_predict(int argc, XsValue* args);
XsValue xs_ai_backpropagate(int argc, XsValue* args);
XsValue xs_ai_setLearningRate(int argc, XsValue* args);
XsValue xs_ai_saveModel(int argc, XsValue* args);
XsValue xs_ai_loadModel(int argc, XsValue* args);

/* Matrix operations */
XsValue xs_ai_createMatrix(int argc, XsValue* args);
XsValue xs_ai_matMul(int argc, XsValue* args);
XsValue xs_ai_matAdd(int argc, XsValue* args);
XsValue xs_ai_matTranspose(int argc, XsValue* args);

/* Activation functions */
XsValue xs_ai_sigmoid(int argc, XsValue* args);
XsValue xs_ai_relu(int argc, XsValue* args);
XsValue xs_ai_softmax(int argc, XsValue* args);

/* Loss functions */
XsValue xs_ai_crossEntropy(int argc, XsValue* args);
XsValue xs_ai_mse(int argc, XsValue* args);

void xs_ai_register(VM* vm);

#endif /* XS_AI_LIB_H */
