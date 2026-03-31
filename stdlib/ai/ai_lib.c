/*
 * X# Standard Library - AI Module Implementation
 * =================================================
 * Real feedforward neural network with backpropagation.
 * Matrix operations use row-major dense storage.
 */

#include "ai_lib.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

/* ===== Matrix (internal) ===== */

typedef struct {
    int    rows;
    int    cols;
    double* data; /* row-major */
} XsMatrix;

static XsMatrix* mat_new(int rows, int cols) {
    XsMatrix* m = (XsMatrix*)calloc(1, sizeof(XsMatrix));
    m->rows = rows;
    m->cols = cols;
    m->data = (double*)calloc(rows * cols, sizeof(double));
    return m;
}

static void mat_free(XsMatrix* m) {
    if (m) { free(m->data); free(m); }
}

static void mat_randomize(XsMatrix* m, double scale) {
    for (int i = 0; i < m->rows * m->cols; i++) {
        m->data[i] = ((double)rand() / RAND_MAX - 0.5) * 2.0 * scale;
    }
}

static XsMatrix* mat_multiply(XsMatrix* a, XsMatrix* b) {
    if (a->cols != b->rows) return NULL;
    XsMatrix* r = mat_new(a->rows, b->cols);
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < b->cols; j++) {
            double sum = 0.0;
            for (int k = 0; k < a->cols; k++) {
                sum += a->data[i * a->cols + k] * b->data[k * b->cols + j];
            }
            r->data[i * r->cols + j] = sum;
        }
    }
    return r;
}

static XsMatrix* mat_add(XsMatrix* a, XsMatrix* b) {
    if (a->rows != b->rows || a->cols != b->cols) return NULL;
    XsMatrix* r = mat_new(a->rows, a->cols);
    for (int i = 0; i < a->rows * a->cols; i++) {
        r->data[i] = a->data[i] + b->data[i];
    }
    return r;
}

static XsMatrix* mat_transpose(XsMatrix* m) {
    XsMatrix* r = mat_new(m->cols, m->rows);
    for (int i = 0; i < m->rows; i++)
        for (int j = 0; j < m->cols; j++)
            r->data[j * r->cols + i] = m->data[i * m->cols + j];
    return r;
}

/* ===== Neural Network (internal) ===== */

#define MAX_LAYERS 32

typedef enum { ACT_SIGMOID, ACT_RELU, ACT_SOFTMAX, ACT_TANH } ActivationType;

typedef struct {
    int           input_size;
    XsMatrix*     weights[MAX_LAYERS];
    XsMatrix*     biases[MAX_LAYERS];
    XsMatrix*     activations[MAX_LAYERS + 1]; /* layer outputs, [0] = input */
    XsMatrix*     z_values[MAX_LAYERS];        /* pre-activation */
    ActivationType act_type[MAX_LAYERS];
    int           layer_sizes[MAX_LAYERS + 1];
    int           num_layers;
    double        learning_rate;
} XsNeuralNet;

static double sigmoid_f(double x) { return 1.0 / (1.0 + exp(-x)); }
static double sigmoid_d(double x) { double s = sigmoid_f(x); return s * (1.0 - s); }
static double relu_f(double x)    { return x > 0 ? x : 0; }
static double relu_d(double x)    { return x > 0 ? 1.0 : 0.0; }
static double tanh_d(double x)    { double t = tanh(x); return 1.0 - t * t; }

static void apply_activation(XsMatrix* z, XsMatrix* out, ActivationType act) {
    int n = z->rows * z->cols;
    switch (act) {
        case ACT_SIGMOID:
            for (int i = 0; i < n; i++) out->data[i] = sigmoid_f(z->data[i]);
            break;
        case ACT_RELU:
            for (int i = 0; i < n; i++) out->data[i] = relu_f(z->data[i]);
            break;
        case ACT_TANH:
            for (int i = 0; i < n; i++) out->data[i] = tanh(z->data[i]);
            break;
        case ACT_SOFTMAX: {
            /* Apply softmax per row */
            for (int r = 0; r < z->rows; r++) {
                double max_val = z->data[r * z->cols];
                for (int c = 1; c < z->cols; c++) {
                    if (z->data[r * z->cols + c] > max_val)
                        max_val = z->data[r * z->cols + c];
                }
                double sum = 0.0;
                for (int c = 0; c < z->cols; c++) {
                    out->data[r * z->cols + c] = exp(z->data[r * z->cols + c] - max_val);
                    sum += out->data[r * z->cols + c];
                }
                for (int c = 0; c < z->cols; c++) {
                    out->data[r * z->cols + c] /= sum;
                }
            }
            break;
        }
    }
}

static double activation_derivative(double z, ActivationType act) {
    switch (act) {
        case ACT_SIGMOID: return sigmoid_d(z);
        case ACT_RELU:    return relu_d(z);
        case ACT_TANH:    return tanh_d(z);
        case ACT_SOFTMAX: return 1.0; /* handled specially in backprop */
    }
    return 1.0;
}

/* ===== Helper: XsMatrix <-> XsArsenal conversions ===== */

static XsMatrix* arsenal_to_matrix(XsArsenal* arr, int rows, int cols) {
    XsMatrix* m = mat_new(rows, cols);
    for (int i = 0; i < rows * cols && i < arr->count; i++) {
        m->data[i] = xs_as_spark(arr->items[i]);
    }
    return m;
}

static XsArsenal* matrix_to_arsenal(XsMatrix* m) {
    XsArsenal* arr = xs_arsenal_new(m->rows * m->cols);
    for (int i = 0; i < m->rows * m->cols; i++) {
        xs_arsenal_push(arr, xs_spark(m->data[i]));
    }
    return arr;
}

/* Store NeuralNet pointer in an entity */
static XsNeuralNet* get_nn(XsValue v) {
    if (v.type != VAL_ENTITY) return NULL;
    XsEntity* e = (XsEntity*)v.object;
    XsValue p = xs_entity_get(e, "_nn_ptr");
    if (p.type == VAL_BLADE) return (XsNeuralNet*)(intptr_t)p.blade;
    return NULL;
}

static XsValue wrap_nn(XsNeuralNet* nn) {
    XsEntity* e = xs_entity_new();
    XsValue p; p.type = VAL_BLADE; p.blade = (int64_t)(intptr_t)nn;
    xs_entity_set(e, "_nn_ptr", p);
    xs_entity_set(e, "layers", xs_blade(nn->num_layers));
    xs_entity_set(e, "lr", xs_spark(nn->learning_rate));
    return xs_entity(e);
}

/* ===== createNeuralNet(inputSize) ===== */

XsValue xs_ai_createNeuralNet(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    static int seeded = 0;
    if (!seeded) { srand((unsigned)time(NULL)); seeded = 1; }

    int input_size = (int)xs_as_spark(args[0]);
    XsNeuralNet* nn = (XsNeuralNet*)calloc(1, sizeof(XsNeuralNet));
    nn->input_size = input_size;
    nn->layer_sizes[0] = input_size;
    nn->num_layers = 0;
    nn->learning_rate = 0.01;
    return wrap_nn(nn);
}

/* ===== addLayer(nn, size, activation) ===== */

XsValue xs_ai_addLayer(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    XsNeuralNet* nn = get_nn(args[0]);
    if (!nn || nn->num_layers >= MAX_LAYERS) return xs_abyss();

    int size = (int)xs_as_spark(args[1]);
    char* act_name = (args[2].type == VAL_SCROLL) ? args[2].scroll : "sigmoid";

    int idx = nn->num_layers;
    int prev_size = nn->layer_sizes[idx];

    /* Xavier initialization */
    double scale = sqrt(2.0 / (prev_size + size));
    nn->weights[idx] = mat_new(prev_size, size);
    mat_randomize(nn->weights[idx], scale);
    nn->biases[idx] = mat_new(1, size);

    if (strcmp(act_name, "relu") == 0)         nn->act_type[idx] = ACT_RELU;
    else if (strcmp(act_name, "softmax") == 0)  nn->act_type[idx] = ACT_SOFTMAX;
    else if (strcmp(act_name, "tanh") == 0)     nn->act_type[idx] = ACT_TANH;
    else                                        nn->act_type[idx] = ACT_SIGMOID;

    nn->num_layers++;
    nn->layer_sizes[nn->num_layers] = size;

    /* Update entity */
    XsEntity* e = (XsEntity*)args[0].object;
    xs_entity_set(e, "layers", xs_blade(nn->num_layers));
    return args[0];
}

/* ===== Forward pass (internal) ===== */

static XsMatrix* nn_forward(XsNeuralNet* nn, XsMatrix* input) {
    /* Free previous activations */
    for (int i = 0; i <= nn->num_layers; i++) {
        if (nn->activations[i]) { mat_free(nn->activations[i]); nn->activations[i] = NULL; }
    }
    for (int i = 0; i < nn->num_layers; i++) {
        if (nn->z_values[i]) { mat_free(nn->z_values[i]); nn->z_values[i] = NULL; }
    }

    /* Copy input as activation[0] */
    nn->activations[0] = mat_new(input->rows, input->cols);
    memcpy(nn->activations[0]->data, input->data, input->rows * input->cols * sizeof(double));

    for (int l = 0; l < nn->num_layers; l++) {
        /* z = a * W + b (broadcast bias) */
        XsMatrix* product = mat_multiply(nn->activations[l], nn->weights[l]);
        if (!product) return NULL;

        nn->z_values[l] = mat_new(product->rows, product->cols);
        for (int r = 0; r < product->rows; r++) {
            for (int c = 0; c < product->cols; c++) {
                nn->z_values[l]->data[r * product->cols + c] =
                    product->data[r * product->cols + c] + nn->biases[l]->data[c];
            }
        }
        mat_free(product);

        nn->activations[l + 1] = mat_new(nn->z_values[l]->rows, nn->z_values[l]->cols);
        apply_activation(nn->z_values[l], nn->activations[l + 1], nn->act_type[l]);
    }
    return nn->activations[nn->num_layers];
}

/* ===== predict(nn, inputArsenal) ===== */

XsValue xs_ai_predict(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsNeuralNet* nn = get_nn(args[0]);
    if (!nn || args[1].type != VAL_ARSENAL) return xs_abyss();

    XsArsenal* in_arr = (XsArsenal*)args[1].object;
    XsMatrix* input = arsenal_to_matrix(in_arr, 1, nn->input_size);
    XsMatrix* output = nn_forward(nn, input);
    mat_free(input);

    if (!output) return xs_abyss();
    XsArsenal* result = matrix_to_arsenal(output);
    return xs_arsenal(result);
}

/* ===== backpropagate(nn, targetArsenal) ===== */

XsValue xs_ai_backpropagate(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsNeuralNet* nn = get_nn(args[0]);
    if (!nn || args[1].type != VAL_ARSENAL) return xs_abyss();
    if (!nn->activations[nn->num_layers]) return xs_abyss();

    XsArsenal* target_arr = (XsArsenal*)args[1].object;
    int out_size = nn->layer_sizes[nn->num_layers];
    XsMatrix* target = arsenal_to_matrix(target_arr, 1, out_size);

    /* Compute output error: delta = (output - target) */
    XsMatrix* output = nn->activations[nn->num_layers];
    XsMatrix** deltas = (XsMatrix**)calloc(nn->num_layers, sizeof(XsMatrix*));

    /* Output layer delta */
    deltas[nn->num_layers - 1] = mat_new(1, out_size);
    for (int j = 0; j < out_size; j++) {
        double err = output->data[j] - target->data[j];
        if (nn->act_type[nn->num_layers - 1] == ACT_SOFTMAX) {
            deltas[nn->num_layers - 1]->data[j] = err;
        } else {
            deltas[nn->num_layers - 1]->data[j] =
                err * activation_derivative(nn->z_values[nn->num_layers - 1]->data[j],
                                           nn->act_type[nn->num_layers - 1]);
        }
    }

    /* Hidden layer deltas (backpropagate) */
    for (int l = nn->num_layers - 2; l >= 0; l--) {
        XsMatrix* wT = mat_transpose(nn->weights[l + 1]);
        XsMatrix* propagated = mat_multiply(deltas[l + 1], wT);
        mat_free(wT);
        if (!propagated) break;

        int size = nn->layer_sizes[l + 1];
        deltas[l] = mat_new(1, size);
        for (int j = 0; j < size; j++) {
            deltas[l]->data[j] = propagated->data[j] *
                activation_derivative(nn->z_values[l]->data[j], nn->act_type[l]);
        }
        mat_free(propagated);
    }

    /* Update weights and biases */
    double lr = nn->learning_rate;
    for (int l = 0; l < nn->num_layers; l++) {
        XsMatrix* a = nn->activations[l];
        XsMatrix* d = deltas[l];
        /* W -= lr * a^T * delta */
        for (int i = 0; i < nn->weights[l]->rows; i++) {
            for (int j = 0; j < nn->weights[l]->cols; j++) {
                nn->weights[l]->data[i * nn->weights[l]->cols + j] -=
                    lr * a->data[i] * d->data[j];
            }
        }
        /* b -= lr * delta */
        for (int j = 0; j < nn->biases[l]->cols; j++) {
            nn->biases[l]->data[j] -= lr * d->data[j];
        }
    }

    /* Compute loss (MSE) */
    double loss = 0.0;
    for (int j = 0; j < out_size; j++) {
        double diff = output->data[j] - target->data[j];
        loss += diff * diff;
    }
    loss /= out_size;

    for (int l = 0; l < nn->num_layers; l++) mat_free(deltas[l]);
    free(deltas);
    mat_free(target);

    return xs_spark(loss);
}

/* ===== train(nn, inputs, targets, epochs) ===== */

XsValue xs_ai_train(int argc, XsValue* args) {
    if (argc < 4) return xs_abyss();
    XsNeuralNet* nn = get_nn(args[0]);
    if (!nn) return xs_abyss();
    XsArsenal* inputs = (args[1].type == VAL_ARSENAL) ? (XsArsenal*)args[1].object : NULL;
    XsArsenal* targets = (args[2].type == VAL_ARSENAL) ? (XsArsenal*)args[2].object : NULL;
    int epochs = (int)xs_as_spark(args[3]);
    if (!inputs || !targets || inputs->count != targets->count) return xs_abyss();

    double final_loss = 0.0;
    int n_samples = inputs->count;

    for (int e = 0; e < epochs; e++) {
        double epoch_loss = 0.0;
        for (int s = 0; s < n_samples; s++) {
            /* Forward */
            XsValue predict_args[2] = { args[0], inputs->items[s] };
            xs_ai_predict(2, predict_args);

            /* Backward */
            XsValue bp_args[2] = { args[0], targets->items[s] };
            XsValue loss = xs_ai_backpropagate(2, bp_args);
            if (loss.type == VAL_SPARK) epoch_loss += loss.spark;
        }
        final_loss = epoch_loss / n_samples;
    }
    return xs_spark(final_loss);
}

/* ===== setLearningRate(nn, lr) ===== */

XsValue xs_ai_setLearningRate(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsNeuralNet* nn = get_nn(args[0]);
    if (!nn) return xs_abyss();
    nn->learning_rate = xs_as_spark(args[1]);
    XsEntity* e = (XsEntity*)args[0].object;
    xs_entity_set(e, "lr", xs_spark(nn->learning_rate));
    return args[0];
}

/* ===== saveModel(nn, path) ===== */

XsValue xs_ai_saveModel(int argc, XsValue* args) {
    if (argc < 2) return xs_fate(false);
    XsNeuralNet* nn = get_nn(args[0]);
    if (!nn || args[1].type != VAL_SCROLL) return xs_fate(false);

    FILE* f = fopen(args[1].scroll, "wb");
    if (!f) return xs_fate(false);

    /* Header */
    fprintf(f, "XSNN %d %d %g\n", nn->input_size, nn->num_layers, nn->learning_rate);
    for (int i = 0; i <= nn->num_layers; i++) fprintf(f, "%d ", nn->layer_sizes[i]);
    fprintf(f, "\n");
    for (int i = 0; i < nn->num_layers; i++) fprintf(f, "%d ", nn->act_type[i]);
    fprintf(f, "\n");

    /* Weights and biases */
    for (int l = 0; l < nn->num_layers; l++) {
        XsMatrix* w = nn->weights[l];
        for (int i = 0; i < w->rows * w->cols; i++) fprintf(f, "%.17g ", w->data[i]);
        fprintf(f, "\n");
        XsMatrix* b = nn->biases[l];
        for (int i = 0; i < b->cols; i++) fprintf(f, "%.17g ", b->data[i]);
        fprintf(f, "\n");
    }
    fclose(f);
    return xs_fate(true);
}

/* ===== loadModel(path) ===== */

XsValue xs_ai_loadModel(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    FILE* f = fopen(args[0].scroll, "rb");
    if (!f) return xs_abyss();

    char magic[8];
    int input_size, num_layers;
    double lr;
    if (fscanf(f, "%4s %d %d %lf", magic, &input_size, &num_layers, &lr) != 4 ||
        strcmp(magic, "XSNN") != 0) {
        fclose(f); return xs_abyss();
    }

    XsNeuralNet* nn = (XsNeuralNet*)calloc(1, sizeof(XsNeuralNet));
    nn->input_size = input_size;
    nn->num_layers = num_layers;
    nn->learning_rate = lr;

    for (int i = 0; i <= num_layers; i++) fscanf(f, "%d", &nn->layer_sizes[i]);
    for (int i = 0; i < num_layers; i++) { int a; fscanf(f, "%d", &a); nn->act_type[i] = (ActivationType)a; }

    for (int l = 0; l < num_layers; l++) {
        int prev = nn->layer_sizes[l];
        int cur  = nn->layer_sizes[l + 1];
        nn->weights[l] = mat_new(prev, cur);
        for (int i = 0; i < prev * cur; i++) fscanf(f, "%lf", &nn->weights[l]->data[i]);
        nn->biases[l] = mat_new(1, cur);
        for (int i = 0; i < cur; i++) fscanf(f, "%lf", &nn->biases[l]->data[i]);
    }
    fclose(f);
    return wrap_nn(nn);
}

/* ===== Matrix operations (exposed) ===== */

XsValue xs_ai_createMatrix(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    int rows = (int)xs_as_spark(args[0]);
    int cols = (int)xs_as_spark(args[1]);
    XsMatrix* m = mat_new(rows, cols);
    if (argc >= 3 && args[2].type == VAL_ARSENAL) {
        XsArsenal* data = (XsArsenal*)args[2].object;
        for (int i = 0; i < rows * cols && i < data->count; i++) {
            m->data[i] = xs_as_spark(data->items[i]);
        }
    }
    /* Store in entity */
    XsEntity* ent = xs_entity_new();
    XsValue p; p.type = VAL_BLADE; p.blade = (int64_t)(intptr_t)m;
    xs_entity_set(ent, "_mat_ptr", p);
    xs_entity_set(ent, "rows", xs_blade(rows));
    xs_entity_set(ent, "cols", xs_blade(cols));
    return xs_entity(ent);
}

static XsMatrix* get_mat(XsValue v) {
    if (v.type != VAL_ENTITY) return NULL;
    XsValue p = xs_entity_get((XsEntity*)v.object, "_mat_ptr");
    if (p.type == VAL_BLADE) return (XsMatrix*)(intptr_t)p.blade;
    return NULL;
}

static XsValue wrap_mat(XsMatrix* m) {
    XsEntity* ent = xs_entity_new();
    XsValue p; p.type = VAL_BLADE; p.blade = (int64_t)(intptr_t)m;
    xs_entity_set(ent, "_mat_ptr", p);
    xs_entity_set(ent, "rows", xs_blade(m->rows));
    xs_entity_set(ent, "cols", xs_blade(m->cols));
    return xs_entity(ent);
}

XsValue xs_ai_matMul(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsMatrix* a = get_mat(args[0]);
    XsMatrix* b = get_mat(args[1]);
    if (!a || !b) return xs_abyss();
    XsMatrix* r = mat_multiply(a, b);
    if (!r) return xs_abyss();
    return wrap_mat(r);
}

XsValue xs_ai_matAdd(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    XsMatrix* a = get_mat(args[0]);
    XsMatrix* b = get_mat(args[1]);
    if (!a || !b) return xs_abyss();
    XsMatrix* r = mat_add(a, b);
    if (!r) return xs_abyss();
    return wrap_mat(r);
}

XsValue xs_ai_matTranspose(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    XsMatrix* m = get_mat(args[0]);
    if (!m) return xs_abyss();
    return wrap_mat(mat_transpose(m));
}

/* ===== Activation functions (standalone) ===== */

XsValue xs_ai_sigmoid(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    if (args[0].type == VAL_ARSENAL) {
        XsArsenal* in = (XsArsenal*)args[0].object;
        XsArsenal* out = xs_arsenal_new(in->count);
        for (int i = 0; i < in->count; i++) {
            xs_arsenal_push(out, xs_spark(sigmoid_f(xs_as_spark(in->items[i]))));
        }
        return xs_arsenal(out);
    }
    return xs_spark(sigmoid_f(xs_as_spark(args[0])));
}

XsValue xs_ai_relu(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    if (args[0].type == VAL_ARSENAL) {
        XsArsenal* in = (XsArsenal*)args[0].object;
        XsArsenal* out = xs_arsenal_new(in->count);
        for (int i = 0; i < in->count; i++) {
            xs_arsenal_push(out, xs_spark(relu_f(xs_as_spark(in->items[i]))));
        }
        return xs_arsenal(out);
    }
    return xs_spark(relu_f(xs_as_spark(args[0])));
}

XsValue xs_ai_softmax(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_ARSENAL) return xs_abyss();
    XsArsenal* in = (XsArsenal*)args[0].object;
    int n = in->count;
    XsArsenal* out = xs_arsenal_new(n);

    /* Find max for numerical stability */
    double max_val = xs_as_spark(in->items[0]);
    for (int i = 1; i < n; i++) {
        double v = xs_as_spark(in->items[i]);
        if (v > max_val) max_val = v;
    }
    double sum = 0.0;
    double* vals = (double*)malloc(n * sizeof(double));
    for (int i = 0; i < n; i++) {
        vals[i] = exp(xs_as_spark(in->items[i]) - max_val);
        sum += vals[i];
    }
    for (int i = 0; i < n; i++) {
        xs_arsenal_push(out, xs_spark(vals[i] / sum));
    }
    free(vals);
    return xs_arsenal(out);
}

/* ===== Loss functions ===== */

XsValue xs_ai_crossEntropy(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    if (args[0].type != VAL_ARSENAL || args[1].type != VAL_ARSENAL) return xs_abyss();
    XsArsenal* pred = (XsArsenal*)args[0].object;
    XsArsenal* target = (XsArsenal*)args[1].object;
    int n = pred->count < target->count ? pred->count : target->count;
    double loss = 0.0;
    for (int i = 0; i < n; i++) {
        double p = xs_as_spark(pred->items[i]);
        double t = xs_as_spark(target->items[i]);
        if (p < 1e-15) p = 1e-15;
        if (p > 1.0 - 1e-15) p = 1.0 - 1e-15;
        loss -= t * log(p) + (1.0 - t) * log(1.0 - p);
    }
    return xs_spark(loss / n);
}

XsValue xs_ai_mse(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    if (args[0].type != VAL_ARSENAL || args[1].type != VAL_ARSENAL) return xs_abyss();
    XsArsenal* pred = (XsArsenal*)args[0].object;
    XsArsenal* target = (XsArsenal*)args[1].object;
    int n = pred->count < target->count ? pred->count : target->count;
    double loss = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = xs_as_spark(pred->items[i]) - xs_as_spark(target->items[i]);
        loss += diff * diff;
    }
    return xs_spark(loss / n);
}

/* ===== Registration ===== */

void xs_ai_register(VM* vm) {
    vm_register_native(vm, "AI.createNeuralNet",  xs_ai_createNeuralNet);
    vm_register_native(vm, "AI.addLayer",         xs_ai_addLayer);
    vm_register_native(vm, "AI.train",            xs_ai_train);
    vm_register_native(vm, "AI.predict",          xs_ai_predict);
    vm_register_native(vm, "AI.backpropagate",    xs_ai_backpropagate);
    vm_register_native(vm, "AI.setLearningRate",  xs_ai_setLearningRate);
    vm_register_native(vm, "AI.saveModel",        xs_ai_saveModel);
    vm_register_native(vm, "AI.loadModel",        xs_ai_loadModel);
    vm_register_native(vm, "AI.createMatrix",     xs_ai_createMatrix);
    vm_register_native(vm, "AI.matMul",           xs_ai_matMul);
    vm_register_native(vm, "AI.matAdd",           xs_ai_matAdd);
    vm_register_native(vm, "AI.matTranspose",     xs_ai_matTranspose);
    vm_register_native(vm, "AI.sigmoid",          xs_ai_sigmoid);
    vm_register_native(vm, "AI.relu",             xs_ai_relu);
    vm_register_native(vm, "AI.softmax",          xs_ai_softmax);
    vm_register_native(vm, "AI.crossEntropy",     xs_ai_crossEntropy);
    vm_register_native(vm, "AI.mse",              xs_ai_mse);
}
