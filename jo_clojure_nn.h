#pragma once
#include <stdio.h> // Added for printf

// Matrix initialization with random values scaled by init_scale
// Arguments: (rows cols init_scale)
static node_idx_t native_nn_init_random_matrix(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    int rows = get_node_int(*it++);
    int cols = get_node_int(*it++);
    double init_scale = it ? get_node_float(*it++) : 0.1; // Default scale if not provided
    
    matrix_ptr_t mat = new_matrix(rows, cols);
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // Random values between -init_scale and init_scale
            double rnd = (jo_pcg32(&jo_rnd_state) / (double)UINT32_MAX * 2.0 - 1.0) * init_scale;
            mat->set(i, j, new_node_float(rnd));
        }
    }
    
    return new_node_matrix(mat);
}

// Initialize bias matrix with alternating values (used to break symmetry)
// Arguments: (rows cols base_value)
static node_idx_t native_nn_init_bias_matrix(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    int rows = get_node_int(*it++);
    int cols = get_node_int(*it++);
    double base_value = it ? get_node_float(*it++) : 0.01; // Default value if not provided
    
    matrix_ptr_t mat = new_matrix(rows, cols);
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double value = (i % 2 == 0) ? base_value : -base_value;
            mat->set(i, j, new_node_float(value));
        }
    }
    
    return new_node_matrix(mat);
}

// Initialize matrix with same value in all cells
// Arguments: (rows cols value)
static node_idx_t native_nn_init_constant_matrix(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    int rows = get_node_int(*it++);
    int cols = get_node_int(*it++);
    double value = it ? get_node_float(*it++) : 0.0; // Default value if not provided
    
    matrix_ptr_t mat = new_matrix(rows, cols);
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            mat->set(i, j, new_node_float(value));
        }
    }
    
    return new_node_matrix(mat);
}

// Initialize with Xavier/Glorot initialization
// Arguments: (rows cols fan_in fan_out)
static node_idx_t native_nn_init_xavier_matrix(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    int rows = get_node_int(*it++);
    int cols = get_node_int(*it++);
    int fan_in = it ? get_node_int(*it++) : cols;  // Default to cols if not provided
    int fan_out = it ? get_node_int(*it++) : rows; // Default to rows if not provided
    
    double scale = sqrt(2.0 / (fan_in + fan_out));
    matrix_ptr_t mat = new_matrix(rows, cols);
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double rnd = (jo_pcg32(&jo_rnd_state) / (double)UINT32_MAX * 2.0 - 1.0) * scale;
            mat->set(i, j, new_node_float(rnd));
        }
    }
    
    return new_node_matrix(mat);
}

// Initialize with He/Kaiming initialization (better for ReLU networks)
// Arguments: (rows cols fan_in)
static node_idx_t native_nn_init_he_matrix(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    int rows = get_node_int(*it++);
    int cols = get_node_int(*it++);
    int fan_in = it ? get_node_int(*it++) : cols; // Default to cols if not provided
    
    double scale = sqrt(2.0 / fan_in);
    matrix_ptr_t mat = new_matrix(rows, cols);
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double rnd = (jo_pcg32(&jo_rnd_state) / (double)UINT32_MAX * 2.0 - 1.0) * scale;
            mat->set(i, j, new_node_float(rnd));
        }
    }
    
    return new_node_matrix(mat);
}

// Linear layer implementation
// Arguments: (in_features out_features)
static node_idx_t native_nn_linear(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    int in_features = get_node_int(*it++);
    int out_features = get_node_int(*it++);
    
    // Standard W: (out_features x in_features) -> Stored as matrix(out_features, in_features)
    matrix_ptr_t weights = new_matrix(out_features, in_features);
    // Standard b: (out_features x 1) -> Stored as matrix(out_features, 1)
    matrix_ptr_t bias = new_matrix(out_features, 1);
    
    // Xavier initialization (scale = sqrt(2 / (in_features + out_features)))
    double scale = jo_math_sqrt(2.0 / (in_features + out_features));
    
    // Initialize weights W[row, col] -> matrix->set(col, row, ...)
    for (int j = 0; j < weights->height /* in_features */; j++) { // row index (input feature)
        for (int i = 0; i < weights->width /* out_features */; i++) { // column index (output feature)
            // Random values between -scale and scale
            double rnd = (jo_pcg32(&jo_rnd_state) / (double)UINT32_MAX * 2.0 - 1.0) * scale;
            // W[k, i] -> set(col=i, row=k=j)
            weights->set(i, j, new_node_float(rnd)); 
        }
    }
    
    // Initialize bias to zeros b[i, 0] -> matrix->set(i, 0, ...)
    for (int i = 0; i < bias->width /* out_features */; i++) {
        bias->set(i, 0, ZERO_NODE); 
    }
    
    // Return a hashmap containing the layer parameters
    hash_map_ptr_t layer = new_hash_map();
    layer->assoc_inplace(new_node_keyword("type"), new_node_keyword("linear"), node_eq);
    layer->assoc_inplace(new_node_keyword("in-features"), new_node_int(in_features), node_eq);
    layer->assoc_inplace(new_node_keyword("out-features"), new_node_int(out_features), node_eq);
    layer->assoc_inplace(new_node_keyword("weights"), new_node_matrix(weights), node_eq);
    layer->assoc_inplace(new_node_keyword("bias"), new_node_matrix(bias), node_eq);
    
    return new_node_hash_map(layer);
}

// Linear layer forward pass
// Arguments: (layer input)
// Calculates Z = X*W + b (where b is broadcast)
// Input X: (batch x in_features) -> matrix(in_features, batch)
// Weights W: (in_features x out_features) -> matrix(out_features, in_features) <- Stored transposed?
// Bias b: (1 x out_features) -> matrix(out_features, 1)
// Output Z: (batch x out_features) -> matrix(out_features, batch)
static node_idx_t native_nn_linear_forward(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t layer_idx = *it++;
    node_idx_t input_idx = *it++; // X
    
    hash_map_ptr_t layer = get_node(layer_idx)->as_hash_map();
    // W is stored as matrix(out, in)
    matrix_ptr_t weights = get_node(layer->get(new_node_keyword("weights"), node_eq))->as_matrix(); 
    // b is stored as matrix(out, 1)
    matrix_ptr_t bias = get_node(layer->get(new_node_keyword("bias"), node_eq))->as_matrix();       
    // X is matrix(in, batch)
    matrix_ptr_t input = get_node(input_idx)->as_matrix();                                         
    
    int batch_size = input->height;
    int in_features = input->width;
    // W is matrix(out, in), so height is in_features
    int layer_in_features = weights->height; 
    int out_features = weights->width; 

    // Check if the input features match layer's input features
    // Check: X.width == W.height
    if (in_features != layer_in_features) { 
        warnf("nn/linear-forward: input features (%d) don't match layer's input features (%d)\n", (int)in_features, (int)layer_in_features); 
        return NIL_NODE;
    }
    
    // Compute Z = W * X + b. 
    // Z is matrix(out, batch), X is matrix(in, batch), W is matrix(out, in), b is matrix(out, 1)
    matrix_ptr_t output = new_matrix(out_features, batch_size); // Z is matrix(out, batch)

    for (int j = 0; j < batch_size; j++) {    // Iterate through batch samples (rows of Z)
        for (int i = 0; i < out_features; i++) { // Iterate through output features (columns of Z)
            double sum = 0.0;
            for (int k = 0; k < in_features; k++) { // Iterate through input features
                // Z(i,j) = sum_k W(i,k) * X(k,j) + B(i,0)
                sum += get_node_float(weights->get(i, k)) * get_node_float(input->get(k, j));
            }
            // Add bias b[i, 0]
            sum += get_node_float(bias->get(i, 0));
            output->set(i, j, new_node_float(sum)); // Set Z[col=i, row=j]
        }
    }
    
    return new_node_matrix(output);
}

// ReLU activation function
// Arguments: (x)
static node_idx_t native_nn_relu(env_ptr_t env, list_ptr_t args) {
    node_idx_t x_idx = args->first_value();
    node_t *x = get_node(x_idx);
    
    if (x->is_float() || x->is_int()) {
        double val = x->as_float();
        return new_node_float(val > 0 ? val : 0);
    }
    else if (x->is_matrix()) {
        matrix_ptr_t matrix = x->as_matrix();
        // Create a matrix with the same dimensions as the input
        matrix_ptr_t result = new_matrix(matrix->width, matrix->height);
        
        for (int i = 0; i < matrix->width; i++) {
            for (int j = 0; j < matrix->height; j++) {
                double val = get_node_float(matrix->get(i, j));
                result->set(i, j, new_node_float(val > 0 ? val : 0));
            }
        }
        
        return new_node_matrix(result);
    }
    else if (x->is_vector()) {
        vector_ptr_t vec = x->as_vector();
        vector_ptr_t result = new_vector();
        
        for (size_t i = 0; i < vec->size(); i++) {
            double val = get_node_float(vec->nth(i));
            result->push_back_inplace(new_node_float(val > 0 ? val : 0));
        }
        
        return new_node_vector(result);
    }
    
    warnf("nn/relu: unsupported input type\n");
    return NIL_NODE;
}

// Sigmoid activation function
// Arguments: (x)
static node_idx_t native_nn_sigmoid(env_ptr_t env, list_ptr_t args) {
    node_idx_t x_idx = args->first_value();
    node_t *x = get_node(x_idx);
    
    if (x->is_float() || x->is_int()) {
        double val = x->as_float();
        return new_node_float(1.0 / (1.0 + exp(-val)));
    }
    else if (x->is_matrix()) {
        matrix_ptr_t matrix = x->as_matrix();
        matrix_ptr_t result = new_matrix(matrix->width, matrix->height);
        
        for (int i = 0; i < matrix->width; i++) {
            for (int j = 0; j < matrix->height; j++) {
                double val = get_node_float(matrix->get(i, j));
                result->set(i, j, new_node_float(1.0 / (1.0 + exp(-val))));
            }
        }
        
        return new_node_matrix(result);
    }
    else if (x->is_vector()) {
        vector_ptr_t vec = x->as_vector();
        vector_ptr_t result = new_vector();
        
        for (size_t i = 0; i < vec->size(); i++) {
            double val = get_node_float(vec->nth(i));
            result->push_back_inplace(new_node_float(1.0 / (1.0 + exp(-val))));
        }
        
        return new_node_vector(result);
    }
    
    warnf("nn/sigmoid: unsupported input type\n");
    return NIL_NODE;
}

// Tanh activation function
// Arguments: (x)
static node_idx_t native_nn_tanh(env_ptr_t env, list_ptr_t args) {
    node_idx_t x_idx = args->first_value();
    node_t *x = get_node(x_idx);
    
    if (x->is_float() || x->is_int()) {
        double val = x->as_float();
        return new_node_float(tanh(val));
    }
    else if (x->is_matrix()) {
        matrix_ptr_t matrix = x->as_matrix();
        matrix_ptr_t result = new_matrix(matrix->width, matrix->height);
        
        for (int i = 0; i < matrix->width; i++) {
            for (int j = 0; j < matrix->height; j++) {
                double val = get_node_float(matrix->get(i, j));
                result->set(i, j, new_node_float(tanh(val)));
            }
        }
        
        return new_node_matrix(result);
    }
    else if (x->is_vector()) {
        vector_ptr_t vec = x->as_vector();
        vector_ptr_t result = new_vector();
        
        for (size_t i = 0; i < vec->size(); i++) {
            double val = get_node_float(vec->nth(i));
            result->push_back_inplace(new_node_float(tanh(val)));
        }
        
        return new_node_vector(result);
    }
    
    warnf("nn/tanh: unsupported input type\n");
    return NIL_NODE;
}

// Softmax activation function
// Arguments: (x)
static node_idx_t native_nn_softmax(env_ptr_t env, list_ptr_t args) {
    node_idx_t x_idx = args->first_value();
    node_t *x = get_node(x_idx);
    
    if (x->is_vector()) {
        vector_ptr_t vec = x->as_vector();
        vector_ptr_t result = new_vector();
        
        // Find max value for numerical stability
        double max_val = -INFINITY;
        for (size_t i = 0; i < vec->size(); i++) {
            double val = get_node_float(vec->nth(i));
            if (val > max_val) max_val = val;
        }
        
        // Compute exp(x - max_val) for each element
        double sum = 0.0;
        vector_ptr_t exp_vec = new_vector();
        for (size_t i = 0; i < vec->size(); i++) {
            double val = get_node_float(vec->nth(i));
            double exp_val = exp(val - max_val);
            sum += exp_val;
            exp_vec->push_back_inplace(new_node_float(exp_val));
        }
        
        // Normalize
        for (size_t i = 0; i < vec->size(); i++) {
            double exp_val = get_node_float(exp_vec->nth(i));
            result->push_back_inplace(new_node_float(exp_val / sum));
        }
        
        return new_node_vector(result);
    }
    else if (x->is_matrix()) {
        // Apply softmax to each row
        matrix_ptr_t matrix = x->as_matrix();
        matrix_ptr_t result = new_matrix(matrix->width, matrix->height);
        
        for (int j = 0; j < matrix->height; j++) {
            // Find max value in row
            double max_val = -INFINITY;
            for (int i = 0; i < matrix->width; i++) {
                double val = get_node_float(matrix->get(i, j));
                if (val > max_val) max_val = val;
            }
            
            // Compute exp(x - max_val) for each element in row
            double sum = 0.0;
            for (int i = 0; i < matrix->width; i++) {
                double val = get_node_float(matrix->get(i, j));
                double exp_val = exp(val - max_val);
                sum += exp_val;
                result->set(i, j, new_node_float(exp_val));
            }
            
            // Normalize
            for (int i = 0; i < matrix->width; i++) {
                double exp_val = get_node_float(result->get(i, j));
                result->set(i, j, new_node_float(exp_val / sum));
            }
        }
        
        return new_node_matrix(result);
    }
    
    warnf("nn/softmax: unsupported input type\n");
    return NIL_NODE;
}

// Mean Squared Error loss function
// Arguments: (predictions targets)
static node_idx_t native_nn_mse_loss(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t pred_idx = *it++;
    node_idx_t targets_idx = *it++;
    
    node_t *pred = get_node(pred_idx);
    node_t *targets = get_node(targets_idx);
    
    if (pred->is_matrix() && targets->is_matrix()) {
        matrix_ptr_t pred_mat = pred->as_matrix();
        matrix_ptr_t targets_mat = targets->as_matrix();
        
        // Check dimensions
        if (pred_mat->width != targets_mat->width || pred_mat->height != targets_mat->height) {
            warnf("nn/mse-loss: prediction and target dimensions don't match\n");
            return NIL_NODE;
        }
        
        double sum_squared_error = 0.0;
        int count = 0;
        
        for (int i = 0; i < pred_mat->width; i++) {
            for (int j = 0; j < pred_mat->height; j++) {
                double pred_val = get_node_float(pred_mat->get(i, j));
                double target_val = get_node_float(targets_mat->get(i, j));
                double error = pred_val - target_val;
                sum_squared_error += error * error;
                count++;
            }
        }
        
        return new_node_float(sum_squared_error / count);
    }
    else if (pred->is_vector() && targets->is_vector()) {
        vector_ptr_t pred_vec = pred->as_vector();
        vector_ptr_t targets_vec = targets->as_vector();
        
        // Check dimensions
        if (pred_vec->size() != targets_vec->size()) {
            warnf("nn/mse-loss: prediction and target dimensions don't match\n");
            return NIL_NODE;
        }
        
        double sum_squared_error = 0.0;
        
        for (size_t i = 0; i < pred_vec->size(); i++) {
            double pred_val = get_node_float(pred_vec->nth(i));
            double target_val = get_node_float(targets_vec->nth(i));
            double error = pred_val - target_val;
            sum_squared_error += error * error;
        }
        
        return new_node_float(sum_squared_error / pred_vec->size());
    }
    
    warnf("nn/mse-loss: unsupported input types\n");
    return NIL_NODE;
}

// Cross entropy loss function
// Arguments: (predictions targets)
static node_idx_t native_nn_cross_entropy_loss(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t pred_idx = *it++;
    node_idx_t targets_idx = *it++;
    
    node_t *pred = get_node(pred_idx);
    node_t *targets = get_node(targets_idx);
    
    if (pred->is_vector() && targets->is_vector()) {
        vector_ptr_t pred_vec = pred->as_vector();
        vector_ptr_t targets_vec = targets->as_vector();
        
        // Check dimensions
        if (pred_vec->size() != targets_vec->size()) {
            warnf("nn/cross-entropy-loss: prediction and target dimensions don't match\n");
            return NIL_NODE;
        }
        
        double loss = 0.0;
        
        for (size_t i = 0; i < pred_vec->size(); i++) {
            double p = get_node_float(pred_vec->nth(i));
            double t = get_node_float(targets_vec->nth(i));
            
            // Clip probability to avoid log(0)
            p = p < 1e-7 ? 1e-7 : (p > 1.0 - 1e-7 ? 1.0 - 1e-7 : p);
            
            loss -= t * log(p) + (1 - t) * log(1 - p);
        }
        
        return new_node_float(loss / pred_vec->size());
    }
    else if (pred->is_matrix() && targets->is_matrix()) {
        matrix_ptr_t pred_mat = pred->as_matrix();
        matrix_ptr_t targets_mat = targets->as_matrix();
        
        // Check dimensions
        if (pred_mat->width != targets_mat->width || pred_mat->height != targets_mat->height) {
            warnf("nn/cross-entropy-loss: prediction and target dimensions don't match\n");
            return NIL_NODE;
        }
        
        double loss = 0.0;
        int count = 0;
        
        for (int i = 0; i < pred_mat->width; i++) {
            for (int j = 0; j < pred_mat->height; j++) {
                double p = get_node_float(pred_mat->get(i, j));
                double t = get_node_float(targets_mat->get(i, j));
                
                // Clip probability to avoid log(0)
                double p_clipped = p < 1e-7 ? 1e-7 : (p > 1.0 - 1e-7 ? 1.0 - 1e-7 : p);
                
                double term = t * log(p_clipped) + (1 - t) * log(1 - p_clipped);
                loss -= term; // Accumulate negated log likelihood
                count++;
            }
        }
        
        double avg_loss = (count > 0) ? (loss / count) : 0.0;
        return new_node_float(avg_loss);
    }
    
    warnf("nn/cross-entropy-loss: unsupported input types\n");
    return NIL_NODE;
}

// SGD Optimizer
// Arguments: (model learning_rate)
static node_idx_t native_nn_sgd(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t model_idx = *it++;
    node_idx_t lr_idx = *it++;
    
    hash_map_ptr_t optimizer = new_hash_map();
    optimizer->assoc_inplace(new_node_keyword("type"), new_node_keyword("sgd"), node_eq);
    optimizer->assoc_inplace(new_node_keyword("learning-rate"), lr_idx, node_eq);
    optimizer->assoc_inplace(new_node_keyword("model"), model_idx, node_eq);
    
    return new_node_hash_map(optimizer);
}

// SGD step - Update model parameters
// Arguments: (optimizer gradients)
static node_idx_t native_nn_sgd_step(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t optimizer_idx = *it++;
    node_idx_t gradients_idx = *it++;
    
    hash_map_ptr_t optimizer = get_node(optimizer_idx)->as_hash_map();
    hash_map_ptr_t gradients = get_node(gradients_idx)->as_hash_map();
    node_idx_t model_idx = optimizer->get(new_node_keyword("model"), node_eq);
    double lr = get_node_float(optimizer->get(new_node_keyword("learning-rate"), node_eq));
    
    // Create updated model copy
    hash_map_ptr_t model = new_hash_map(*get_node(model_idx)->as_hash_map());
    
    // Iterate through layers in the model
    for (hash_map_t::iterator layer_it = model->begin(); layer_it; ++layer_it) {
        node_idx_t layer_key = layer_it->first;
        node_idx_t layer_node_idx = layer_it->second;
        node_t *layer_node = get_node(layer_node_idx);
        
        if (!layer_node->is_hash_map()) continue;
        hash_map_ptr_t layer_params = layer_node->as_hash_map();
        
        // Get gradients for this layer
        node_idx_t layer_grad_idx = gradients->get(layer_key, node_eq);
        if (layer_grad_idx == INV_NODE) continue;
        hash_map_ptr_t layer_grads = get_node(layer_grad_idx)->as_hash_map();
        
        // Copy layer parameters for update
        hash_map_ptr_t updated_layer_params = new_hash_map(*layer_params);
        
        // Update each parameter within this layer
        for (hash_map_t::iterator param_it = layer_params->begin(); param_it; ++param_it) {
            node_idx_t param_key = param_it->first;
            node_idx_t param_idx = param_it->second;
            node_idx_t grad_idx = layer_grads->get(param_key, node_eq);
            if (grad_idx == INV_NODE) continue;
            
            node_t *param_node = get_node(param_idx);
            node_t *grad_node = get_node(grad_idx);
            if (!param_node->is_matrix() || !grad_node->is_matrix()) continue;
            
            matrix_ptr_t param_mat = param_node->as_matrix();
            matrix_ptr_t grad_mat = grad_node->as_matrix();
            matrix_ptr_t updated_mat = new_matrix(param_mat->width, param_mat->height);
            
            for (int i = 0; i < param_mat->width; ++i) {
                for (int j = 0; j < param_mat->height; ++j) {
                    double p = get_node_float(param_mat->get(i, j));
                    double g = get_node_float(grad_mat->get(i, j));
                    updated_mat->set(i, j, new_node_float(p - lr * g));
                }
            }
            updated_layer_params->assoc_inplace(param_key, new_node_matrix(updated_mat), node_eq);
        }
        
        // Place updated layer back into model
        model->assoc_inplace(layer_key, new_node_hash_map(updated_layer_params), node_eq);
    }
    
    // Update model in optimizer
    hash_map_ptr_t updated_optimizer = new_hash_map(*optimizer);
    updated_optimizer->assoc_inplace(new_node_keyword("model"), new_node_hash_map(model), node_eq);
    return new_node_hash_map(updated_optimizer);
}

// Adam Optimizer
// Arguments: (model learning_rate beta1 beta2 epsilon)
static node_idx_t native_nn_adam(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t model_idx = *it++;
    node_idx_t lr_idx = *it++;
    
    double beta1 = 0.9, beta2 = 0.999, epsilon = 1e-8;
    if (it) beta1 = get_node_float(*it++);
    if (it) beta2 = get_node_float(*it++);
    if (it) epsilon = get_node_float(*it++);
    
    hash_map_ptr_t optimizer = new_hash_map();
    optimizer->assoc_inplace(new_node_keyword("type"), new_node_keyword("adam"), node_eq);
    optimizer->assoc_inplace(new_node_keyword("learning-rate"), lr_idx, node_eq);
    optimizer->assoc_inplace(new_node_keyword("beta1"), new_node_float(beta1), node_eq);
    optimizer->assoc_inplace(new_node_keyword("beta2"), new_node_float(beta2), node_eq);
    optimizer->assoc_inplace(new_node_keyword("epsilon"), new_node_float(epsilon), node_eq);
    optimizer->assoc_inplace(new_node_keyword("model"), model_idx, node_eq);
    optimizer->assoc_inplace(new_node_keyword("t"), ONE_NODE, node_eq);
    
    // Initialize moment estimates (m and v) with nested structure
    hash_map_ptr_t m = new_hash_map();
    hash_map_ptr_t v = new_hash_map();
    
    hash_map_ptr_t model = get_node(model_idx)->as_hash_map();
    // Iterate through layers (:layer1, :layer2)
    for (hash_map_t::iterator layer_it = model->begin(); layer_it; layer_it++) {
        node_idx_t layer_key = layer_it->first;
        node_t* layer_node = get_node(layer_it->second);
        
        if (layer_node->is_hash_map()) { // Check if the value is a layer map
             hash_map_ptr_t layer_params = layer_node->as_hash_map();
             hash_map_ptr_t layer_m = new_hash_map(); // Moment map for this layer
             hash_map_ptr_t layer_v = new_hash_map(); // Moment map for this layer

             // Iterate through params within the layer (:weights, :bias)
             for (hash_map_t::iterator param_it = layer_params->begin(); param_it; param_it++) {
                 node_idx_t param_key = param_it->first;
                 node_t* param_node = get_node(param_it->second);
                 
                 if (param_node->is_matrix()) { // Check if the parameter is a matrix
                     matrix_ptr_t param_mat = param_node->as_matrix();
                     matrix_ptr_t m_mat = new_matrix(param_mat->width, param_mat->height);
                     matrix_ptr_t v_mat = new_matrix(param_mat->width, param_mat->height);

                     // Initialize moment matrices to zero
                     for (int i = 0; i < param_mat->width; i++) {
                         for (int j = 0; j < param_mat->height; j++) {
                             m_mat->set(i, j, ZERO_NODE);
                             v_mat->set(i, j, ZERO_NODE);
                         }
                     }
                     // Add the zeroed moment matrix to the layer's moment map
                     layer_m->assoc_inplace(param_key, new_node_matrix(m_mat), node_eq);
                     layer_v->assoc_inplace(param_key, new_node_matrix(v_mat), node_eq);
                 }
             }
             // Add the completed layer moment map to the main m and v maps
             m->assoc_inplace(layer_key, new_node_hash_map(layer_m), node_eq);
             v->assoc_inplace(layer_key, new_node_hash_map(layer_v), node_eq);
        }
    }
    
    optimizer->assoc_inplace(new_node_keyword("m"), new_node_hash_map(m), node_eq);
    optimizer->assoc_inplace(new_node_keyword("v"), new_node_hash_map(v), node_eq);
    
    return new_node_hash_map(optimizer);
}

// Adam step - Update model parameters
// Arguments: (optimizer gradients)
static node_idx_t native_nn_adam_step(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t optimizer_idx = *it++;
    node_idx_t gradients_idx = *it++;
    
    hash_map_ptr_t optimizer = get_node(optimizer_idx)->as_hash_map();
    // gradients map is now nested: {:layer1 {:weights G, :bias G}, ...}
    hash_map_ptr_t gradients = get_node(gradients_idx)->as_hash_map(); 
    node_idx_t model_idx = optimizer->get(new_node_keyword("model"), node_eq);
    double lr = get_node_float(optimizer->get(new_node_keyword("learning-rate"), node_eq));
    double beta1 = get_node_float(optimizer->get(new_node_keyword("beta1"), node_eq));
    double beta2 = get_node_float(optimizer->get(new_node_keyword("beta2"), node_eq));
    double epsilon = get_node_float(optimizer->get(new_node_keyword("epsilon"), node_eq));
    int t = get_node_int(optimizer->get(new_node_keyword("t"), node_eq));
    
    // m and v maps mirror the model structure
    hash_map_ptr_t m = get_node(optimizer->get(new_node_keyword("m"), node_eq))->as_hash_map();
    hash_map_ptr_t v = get_node(optimizer->get(new_node_keyword("v"), node_eq))->as_hash_map();
    
    // Create copies for the updated state
    hash_map_ptr_t model = new_hash_map(*get_node(model_idx)->as_hash_map());
    hash_map_ptr_t m_new = new_hash_map(); // Start fresh for updated moments
    hash_map_ptr_t v_new = new_hash_map(); // Start fresh for updated moments
    
    // Iterate through layers in the model (e.g., :layer1, :layer2)
    for (hash_map_t::iterator layer_it = model->begin(); layer_it; layer_it++) {
        node_idx_t layer_key = layer_it->first;
        node_idx_t layer_node_idx = layer_it->second;
        node_t* layer_node = get_node(layer_node_idx);

        if (!layer_node->is_hash_map()) continue;
        hash_map_ptr_t layer_params = layer_node->as_hash_map();
        
        // Get the corresponding gradients, m, and v for this layer
        node_idx_t layer_gradients_idx = gradients->get(layer_key, node_eq);
        node_idx_t layer_m_idx = m->get(layer_key, node_eq);
        node_idx_t layer_v_idx = v->get(layer_key, node_eq);

        // Check if gradients/moments exist for this layer
        if (layer_gradients_idx == INV_NODE || layer_m_idx == INV_NODE || layer_v_idx == INV_NODE) continue;
        
        hash_map_ptr_t layer_gradients = get_node(layer_gradients_idx)->as_hash_map();
        hash_map_ptr_t layer_m = get_node(layer_m_idx)->as_hash_map();
        hash_map_ptr_t layer_v = get_node(layer_v_idx)->as_hash_map();

        // Create maps to hold updated params and moments for this layer
        hash_map_ptr_t updated_layer_params = new_hash_map(*layer_params);
        hash_map_ptr_t updated_layer_m = new_hash_map();
        hash_map_ptr_t updated_layer_v = new_hash_map();

        // Iterate through parameters within the layer (e.g., :weights, :bias)
        for (hash_map_t::iterator param_it = layer_params->begin(); param_it; param_it++) {
            node_idx_t param_key = param_it->first;
            node_idx_t param = param_it->second;
            
            // Look up gradient and moments using param_key within the layer's maps
            node_idx_t grad = layer_gradients->get(param_key, node_eq);
            node_idx_t m_param = layer_m->get(param_key, node_eq);
            node_idx_t v_param = layer_v->get(param_key, node_eq);

            // Skip if no gradient or moments found for this specific parameter
            if (grad == INV_NODE || m_param == INV_NODE || v_param == INV_NODE) continue;
            
            node_t *param_node = get_node(param);
            node_t *grad_node = get_node(grad);
            node_t *m_node = get_node(m_param);
            node_t *v_node = get_node(v_param);
            
            if (param_node->is_matrix() && grad_node->is_matrix() && 
                m_node->is_matrix() && v_node->is_matrix()) {
                
                matrix_ptr_t param_mat = param_node->as_matrix();
                matrix_ptr_t grad_mat = grad_node->as_matrix();
                matrix_ptr_t m_mat = m_node->as_matrix();
                matrix_ptr_t v_mat = v_node->as_matrix();
                
                matrix_ptr_t m_new_mat = new_matrix(m_mat->width, m_mat->height);
                matrix_ptr_t v_new_mat = new_matrix(v_mat->width, v_mat->height);
                matrix_ptr_t updated_param_mat = new_matrix(param_mat->width, param_mat->height);
                
                for (int i = 0; i < param_mat->width; i++) {
                    for (int j = 0; j < param_mat->height; j++) {
                        double p = get_node_float(param_mat->get(i, j));
                        double g = get_node_float(grad_mat->get(i, j));
                        double m_val = get_node_float(m_mat->get(i, j));
                        double v_val = get_node_float(v_mat->get(i, j));
                        
                        double m_new_val = beta1 * m_val + (1 - beta1) * g;
                        m_new_mat->set(i, j, new_node_float(m_new_val));
                        
                        double v_new_val = beta2 * v_val + (1 - beta2) * g * g;
                        v_new_mat->set(i, j, new_node_float(v_new_val));
                        
                        double m_hat = m_new_val / (1 - pow(beta1, t));
                        double v_hat = v_new_val / (1 - pow(beta2, t));
                        
                        double param_new = p - lr * m_hat / (sqrt(v_hat) + epsilon);
                        updated_param_mat->set(i, j, new_node_float(param_new));
                    }
                }
                
                // Update the parameter within the layer's updated map
                updated_layer_params->assoc_inplace(param_key, new_node_matrix(updated_param_mat), node_eq);
                // Store the new moments in the layer's updated moment maps
                updated_layer_m->assoc_inplace(param_key, new_node_matrix(m_new_mat), node_eq);
                updated_layer_v->assoc_inplace(param_key, new_node_matrix(v_new_mat), node_eq);
            }
        }
        // Update the layer in the main model map
        model->assoc_inplace(layer_key, new_node_hash_map(updated_layer_params), node_eq);
        // Store the updated moments for the layer
        m_new->assoc_inplace(layer_key, new_node_hash_map(updated_layer_m), node_eq);
        v_new->assoc_inplace(layer_key, new_node_hash_map(updated_layer_v), node_eq);
    }
    
    // Update optimizer state
    hash_map_ptr_t updated_optimizer = new_hash_map(*optimizer);
    updated_optimizer->assoc_inplace(new_node_keyword("model"), new_node_hash_map(model), node_eq);
    updated_optimizer->assoc_inplace(new_node_keyword("m"), new_node_hash_map(m_new), node_eq);
    updated_optimizer->assoc_inplace(new_node_keyword("v"), new_node_hash_map(v_new), node_eq);
    updated_optimizer->assoc_inplace(new_node_keyword("t"), new_node_int(t + 1), node_eq);
    
    return new_node_hash_map(updated_optimizer);
}

// Analytical Backpropagation (Standard Dimensions Storage, Standard Mul, Derived Formulas)
// Arguments: (model X y)
// model: The model hash-map {:layer1 {...} :layer2 {...} ...}
// X: Input matrix (batch x in_features) -> matrix(in_features, batch)
// y: Target matrix (batch x out_features) -> matrix(out_features, batch)
// Returns: Nested gradient map {:layer1 {:weights dW1 :bias db1} ...}
static node_idx_t native_nn_backward(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t model_idx = *it++;
    node_idx_t X_idx = *it++;
    node_idx_t y_idx = *it++;

    hash_map_ptr_t model = get_node(model_idx)->as_hash_map();
    matrix_ptr_t X = get_node(X_idx)->as_matrix(); // matrix(in, batch)
    matrix_ptr_t y = get_node(y_idx)->as_matrix(); // matrix(out, batch)

    int batch_size = X->height;
    double scale_factor = 1.0 / batch_size; // For averaging gradients

    // --- Forward Pass (with caching) ---
    hash_map_ptr_t cache = new_hash_map();
    cache->assoc_inplace(new_node_keyword("A0"), X_idx, node_eq); // Input is A0: matrix(in, batch)

    // Detect network layers and structure
    vector_ptr_t layer_keys = new_vector();
    int layer_count = 0;
    
    // Find all layer keys and sort them
    for (hash_map_t::iterator model_it = model->begin(); model_it; model_it++) {
        node_idx_t key = model_it->first;
        node_t* key_node = get_node(key);
        
        // Only process layers that are named in the format "layerX" where X is a number
        if (key_node->type == NODE_KEYWORD) {
            const char* key_str = key_node->t_string.c_str();
            if (strncmp(key_str, "layer", 5) == 0) {
                layer_keys->push_back_inplace(key);
                layer_count++;
            }
        }
    }
    
    // Sort layer keys in ascending order
    // (Simple bubble sort for simplicity)
    for (int i = 0; i < layer_keys->size(); i++) {
        for (int j = i + 1; j < layer_keys->size(); j++) {
            const char* key_i = get_node(layer_keys->nth(i))->t_string.c_str();
            const char* key_j = get_node(layer_keys->nth(j))->t_string.c_str();
            
            // Extract and compare layer numbers
            int num_i = atoi(key_i + 5); // +5 to skip "layer"
            int num_j = atoi(key_j + 5);
            
            if (num_i > num_j) {
                // Swap
                node_idx_t temp = layer_keys->nth(i);
                vector_ptr_t new_keys = layer_keys->assoc(i, layer_keys->nth(j));
                new_keys = new_keys->assoc(j, temp);
                layer_keys = new_keys;
            }
        }
    }
    
    // --- Forward Pass (with caching) ---
    node_idx_t current_A = X_idx; // Current activation starts with input X

    // Process each layer in order
    for (int i = 0; i < layer_keys->size(); i++) {
        node_idx_t layer_key = layer_keys->nth(i);
        const char* layer_name = get_node(layer_key)->t_string.c_str();

        // Get the layer parameters
        node_idx_t layer_params_idx = model->get(layer_key, node_eq);
        hash_map_ptr_t layer_params = get_node(layer_params_idx)->as_hash_map();
        
        // Get layer type (default to "linear" if not specified)
        node_idx_t type_idx = layer_params->get(new_node_keyword("type"), node_eq);
        const char* layer_type = (type_idx != INV_NODE) ? get_node(type_idx)->t_string.c_str() : "linear";

        // Get weights matrix
        matrix_ptr_t W = get_node(layer_params->get(new_node_keyword("weights"), node_eq))->as_matrix();
        
        // Perform linear forward pass
        node_idx_t Z_idx = native_nn_linear_forward(env, list_va(layer_params_idx, current_A));
        
        // Cache the weights and Z values
        char W_cache_key[64];
        char Z_cache_key[64];
        char A_cache_key[64];
        
        sprintf(W_cache_key, "%s/W", layer_name);
        sprintf(Z_cache_key, "%s/Z", layer_name);
        sprintf(A_cache_key, "%s/A", layer_name);
        
        cache->assoc_inplace(new_node_keyword(W_cache_key), new_node_matrix(W), node_eq);
        cache->assoc_inplace(new_node_keyword(Z_cache_key), Z_idx, node_eq);
        
        // Apply activation function based on layer type or position
        node_idx_t A_idx;
        node_idx_t activation_key = layer_params->get(new_node_keyword("activation"), node_eq);
        const char* activation_type = NULL;

        // Check if the activation is specified in the layer parameters
        if (activation_key != INV_NODE) {
            activation_type = get_node(activation_key)->t_string.c_str();
        } else {
            // Default activation: relu for hidden layers, sigmoid for output layer
            activation_type = (i < layer_keys->size() - 1) ? "relu" : "sigmoid";
        }
        
        // Apply the activation function
        if (strcmp(activation_type, "relu") == 0) {
            A_idx = native_nn_relu(env, list_va(Z_idx));
        } else if (strcmp(activation_type, "sigmoid") == 0) {
            A_idx = native_nn_sigmoid(env, list_va(Z_idx));
        } else if (strcmp(activation_type, "tanh") == 0) {
            A_idx = native_nn_tanh(env, list_va(Z_idx));
        } else if (strcmp(activation_type, "softmax") == 0) {
            A_idx = native_nn_softmax(env, list_va(Z_idx));
        } else {
            // Default to linear (no activation)
            A_idx = Z_idx;
        }
        
        // Cache the activation output
        cache->assoc_inplace(new_node_keyword(A_cache_key), A_idx, node_eq);
        
        // Update current activation for next layer
        current_A = A_idx;
    }

    // --- Backward Pass ---
    hash_map_ptr_t gradients = new_hash_map();

    // Initial gradient calculation depends on the last activation function
    // For now, we assume binary cross-entropy (A - y) for sigmoid output
    // In a more complete implementation, this should be determined by the loss function
    node_idx_t last_layer_key = layer_keys->nth(layer_keys->size() - 1);
    const char* last_layer_name = get_node(last_layer_key)->t_string.c_str();
    char last_A_cache_key[64];
    sprintf(last_A_cache_key, "%s/A", last_layer_name);
    node_idx_t last_A_idx = cache->get(new_node_keyword(last_A_cache_key), node_eq);
    
    // Initial gradient dZ for output layer: A - y
    node_idx_t dZ_curr_idx = native_math_matrix_sub(env, list_va(last_A_idx, y_idx));
    
    // Process each layer in reverse order
    for (int i = layer_keys->size() - 1; i >= 0; i--) {
        node_idx_t layer_key = layer_keys->nth(i);
        const char* layer_name = get_node(layer_key)->t_string.c_str();
        
        // Get layer parameters and activation type
        node_idx_t layer_params_idx = model->get(layer_key, node_eq);
        hash_map_ptr_t layer_params = get_node(layer_params_idx)->as_hash_map();
        
        char W_cache_key[64];
        char Z_cache_key[64];
        char A_cache_key[64];
        char prev_A_cache_key[64];
        
        sprintf(W_cache_key, "%s/W", layer_name);
        sprintf(Z_cache_key, "%s/Z", layer_name);
        sprintf(A_cache_key, "%s/A", layer_name);
        
        // Get the activation for the previous layer (or input X for first layer)
        if (i > 0) {
            const char* prev_layer_name = get_node(layer_keys->nth(i-1))->t_string.c_str();
            sprintf(prev_A_cache_key, "%s/A", prev_layer_name);
        } else {
            strcpy(prev_A_cache_key, "A0");
        }
        
        // Get cached values
        matrix_ptr_t W = get_node(cache->get(new_node_keyword(W_cache_key), node_eq))->as_matrix();
        node_idx_t Z_idx = cache->get(new_node_keyword(Z_cache_key), node_eq);
        node_idx_t A_prev_idx = cache->get(new_node_keyword(prev_A_cache_key), node_eq);
        
        // 1. Calculate gradients for weights and bias
        // dW = (1/m) * mul(transpose(A_prev), dZ)
        // db = (1/m) * sum_rows(dZ)
        node_idx_t A_prev_T_idx = native_math_matrix_transpose(env, list_va(A_prev_idx));
        node_idx_t dW_unscaled_idx = native_math_matrix_mul(env, list_va(A_prev_T_idx, dZ_curr_idx));
        node_idx_t dW_idx = native_math_matrix_scale(env, list_va(dW_unscaled_idx, new_node_float(scale_factor)));
        node_idx_t db_unscaled_idx = native_math_matrix_sum_rows(env, list_va(dZ_curr_idx));
        node_idx_t db_idx = native_math_matrix_scale(env, list_va(db_unscaled_idx, new_node_float(scale_factor)));
        
        // Store gradients for this layer
        hash_map_ptr_t layer_grads = new_hash_map();
        layer_grads->assoc_inplace(new_node_keyword("weights"), dW_idx, node_eq);
        layer_grads->assoc_inplace(new_node_keyword("bias"), db_idx, node_eq);
        gradients->assoc_inplace(layer_key, new_node_hash_map(layer_grads), node_eq);
        
        // Prepare for next layer (if not at the input)
        if (i > 0) {
            // Calculate dA_prev
            node_idx_t W_T_idx = native_math_matrix_transpose(env, list_va(new_node_matrix(W)));
            node_idx_t dA_prev_idx = native_math_matrix_mul(env, list_va(dZ_curr_idx, W_T_idx));
            
            // Apply activation gradient for the previous layer
            node_idx_t activation_key = layer_params->get(new_node_keyword("activation"), node_eq);
            const char* activation_type = NULL;
            
            // Determine the activation type of the previous layer
            node_idx_t prev_layer_key = layer_keys->nth(i-1);
            node_idx_t prev_layer_params_idx = model->get(prev_layer_key, node_eq);
            hash_map_ptr_t prev_layer_params = get_node(prev_layer_params_idx)->as_hash_map();
            node_idx_t prev_activation_key = prev_layer_params->get(new_node_keyword("activation"), node_eq);
            
            if (prev_activation_key != INV_NODE) {
                activation_type = get_node(prev_activation_key)->t_string.c_str();
            } else {
                // Default activation: relu for hidden layers
                activation_type = "relu";
            }
            
            // Get the previous layer's Z values
            char prev_Z_cache_key[64];
            const char* prev_layer_name = get_node(layer_keys->nth(i-1))->t_string.c_str();
            sprintf(prev_Z_cache_key, "%s/Z", prev_layer_name);
            node_idx_t prev_Z_idx = cache->get(new_node_keyword(prev_Z_cache_key), node_eq);
            matrix_ptr_t prev_Z = get_node(prev_Z_idx)->as_matrix();
            
            // Apply backprop through activation function
            matrix_ptr_t dA_prev = get_node(dA_prev_idx)->as_matrix();
            matrix_ptr_t dZ_prev;
            
            if (strcmp(activation_type, "relu") == 0) {
                // ReLU derivative: f'(z) = z > 0 ? 1 : 0
                dZ_prev = new_matrix(prev_Z->width, prev_Z->height);
                for (int row = 0; row < prev_Z->width; row++) {
                    for (int col = 0; col < prev_Z->height; col++) {
                        double z_val = get_node_float(prev_Z->get(row, col));
                        double da_val = get_node_float(dA_prev->get(row, col));
                        dZ_prev->set(row, col, new_node_float(z_val > 0 ? da_val : 0.0));
                    }
                }
            } else if (strcmp(activation_type, "sigmoid") == 0) {
                // Sigmoid derivative: f'(z) = sigmoid(z) * (1 - sigmoid(z))
                node_idx_t prev_A_idx = cache->get(new_node_keyword(prev_A_cache_key), node_eq);
                matrix_ptr_t prev_A = get_node(prev_A_idx)->as_matrix();
                dZ_prev = new_matrix(prev_Z->width, prev_Z->height);
                for (int row = 0; row < prev_Z->width; row++) {
                    for (int col = 0; col < prev_Z->height; col++) {
                        double a_val = get_node_float(prev_A->get(row, col));
                        double da_val = get_node_float(dA_prev->get(row, col));
                        double dsigmoid = a_val * (1.0 - a_val);
                        dZ_prev->set(row, col, new_node_float(da_val * dsigmoid));
                    }
                }
            } else if (strcmp(activation_type, "tanh") == 0) {
                // Tanh derivative: f'(z) = 1 - tanh(z)^2
                node_idx_t prev_A_idx = cache->get(new_node_keyword(prev_A_cache_key), node_eq);
                matrix_ptr_t prev_A = get_node(prev_A_idx)->as_matrix();
                dZ_prev = new_matrix(prev_Z->width, prev_Z->height);
                for (int row = 0; row < prev_Z->width; row++) {
                    for (int col = 0; col < prev_Z->height; col++) {
                        double a_val = get_node_float(prev_A->get(row, col));
                        double da_val = get_node_float(dA_prev->get(row, col));
                        double dtanh = 1.0 - (a_val * a_val);
                        dZ_prev->set(row, col, new_node_float(da_val * dtanh));
                    }
                }
            } else {
                // Default to linear (derivative = 1)
                dZ_prev = new_matrix(*dA_prev);
            }
            
            // Update current dZ for next iteration
            dZ_curr_idx = new_node_matrix(dZ_prev);
        }
    }
    
    return new_node_hash_map(gradients);
}

// LeakyReLU activation function with customizable negative slope
// Arguments: (x alpha)
static node_idx_t native_nn_leaky_relu(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t x_idx = *it++;
    node_t *x = get_node(x_idx);
    double alpha = it ? get_node_float(*it++) : 0.01; // Default alpha = 0.01
    
    if (x->is_float() || x->is_int()) {
        double val = x->as_float();
        return new_node_float(val > 0 ? val : alpha * val);
    }
    else if (x->is_matrix()) {
        matrix_ptr_t matrix = x->as_matrix();
        matrix_ptr_t result = new_matrix(matrix->width, matrix->height);
        
        for (int i = 0; i < matrix->width; i++) {
            for (int j = 0; j < matrix->height; j++) {
                double val = get_node_float(matrix->get(i, j));
                result->set(i, j, new_node_float(val > 0 ? val : alpha * val));
            }
        }
        
        return new_node_matrix(result);
    }
    else if (x->is_vector()) {
        vector_ptr_t vec = x->as_vector();
        vector_ptr_t result = new_vector();
        
        for (size_t i = 0; i < vec->size(); i++) {
            double val = get_node_float(vec->nth(i));
            result->push_back_inplace(new_node_float(val > 0 ? val : alpha * val));
        }
        
        return new_node_vector(result);
    }
    
    warnf("nn/leaky-relu: unsupported input type\n");
    return NIL_NODE;
}

// GELU activation function (Gaussian Error Linear Unit)
// Arguments: (x)
static node_idx_t native_nn_gelu(env_ptr_t env, list_ptr_t args) {
    node_idx_t x_idx = args->first_value();
    node_t *x = get_node(x_idx);
    
    if (x->is_float() || x->is_int()) {
        double val = x->as_float();
        // Approximation: 0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3)))
        double x3 = val * val * val;
        double inner = sqrt(2.0/M_PI) * (val + 0.044715 * x3);
        return new_node_float(0.5 * val * (1.0 + tanh(inner)));
    }
    else if (x->is_matrix()) {
        matrix_ptr_t matrix = x->as_matrix();
        matrix_ptr_t result = new_matrix(matrix->width, matrix->height);
        
        for (int i = 0; i < matrix->width; i++) {
            for (int j = 0; j < matrix->height; j++) {
                double val = get_node_float(matrix->get(i, j));
                double x3 = val * val * val;
                double inner = sqrt(2.0/M_PI) * (val + 0.044715 * x3);
                result->set(i, j, new_node_float(0.5 * val * (1.0 + tanh(inner))));
            }
        }
        
        return new_node_matrix(result);
    }
    else if (x->is_vector()) {
        vector_ptr_t vec = x->as_vector();
        vector_ptr_t result = new_vector();
        
        for (size_t i = 0; i < vec->size(); i++) {
            double val = get_node_float(vec->nth(i));
            double x3 = val * val * val;
            double inner = sqrt(2.0/M_PI) * (val + 0.044715 * x3);
            result->push_back_inplace(new_node_float(0.5 * val * (1.0 + tanh(inner))));
        }
        
        return new_node_vector(result);
    }
    
    warnf("nn/gelu: unsupported input type\n");
    return NIL_NODE;
}

// ELU (Exponential Linear Unit) activation function
// Arguments: (x alpha)
static node_idx_t native_nn_elu(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t x_idx = *it++;
    node_t *x = get_node(x_idx);
    double alpha = it ? get_node_float(*it++) : 1.0; // Default alpha = 1.0
    
    if (x->is_float() || x->is_int()) {
        double val = x->as_float();
        return new_node_float(val > 0 ? val : alpha * (exp(val) - 1.0));
    }
    else if (x->is_matrix()) {
        matrix_ptr_t matrix = x->as_matrix();
        matrix_ptr_t result = new_matrix(matrix->width, matrix->height);
        
        for (int i = 0; i < matrix->width; i++) {
            for (int j = 0; j < matrix->height; j++) {
                double val = get_node_float(matrix->get(i, j));
                result->set(i, j, new_node_float(val > 0 ? val : alpha * (exp(val) - 1.0)));
            }
        }
        
        return new_node_matrix(result);
    }
    else if (x->is_vector()) {
        vector_ptr_t vec = x->as_vector();
        vector_ptr_t result = new_vector();
        
        for (size_t i = 0; i < vec->size(); i++) {
            double val = get_node_float(vec->nth(i));
            result->push_back_inplace(new_node_float(val > 0 ? val : alpha * (exp(val) - 1.0)));
        }
        
        return new_node_vector(result);
    }
    
    warnf("nn/elu: unsupported input type\n");
    return NIL_NODE;
}

// Swish/SiLU activation function (Sigmoid Linear Unit)
// Arguments: (x beta)
static node_idx_t native_nn_swish(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t x_idx = *it++;
    node_t *x = get_node(x_idx);
    double beta = it ? get_node_float(*it++) : 1.0; // Default beta = 1.0
    
    if (x->is_float() || x->is_int()) {
        double val = x->as_float();
        double sigmoid = 1.0 / (1.0 + exp(-beta * val));
        return new_node_float(val * sigmoid);
    }
    else if (x->is_matrix()) {
        matrix_ptr_t matrix = x->as_matrix();
        matrix_ptr_t result = new_matrix(matrix->width, matrix->height);
        
        for (int i = 0; i < matrix->width; i++) {
            for (int j = 0; j < matrix->height; j++) {
                double val = get_node_float(matrix->get(i, j));
                double sigmoid = 1.0 / (1.0 + exp(-beta * val));
                result->set(i, j, new_node_float(val * sigmoid));
            }
        }
        
        return new_node_matrix(result);
    }
    else if (x->is_vector()) {
        vector_ptr_t vec = x->as_vector();
        vector_ptr_t result = new_vector();
        
        for (size_t i = 0; i < vec->size(); i++) {
            double val = get_node_float(vec->nth(i));
            double sigmoid = 1.0 / (1.0 + exp(-beta * val));
            result->push_back_inplace(new_node_float(val * sigmoid));
        }
        
        return new_node_vector(result);
    }
    
    warnf("nn/swish: unsupported input type\n");
    return NIL_NODE;
}

// Dropout layer implementation
// Arguments: (drop_prob)
static node_idx_t native_nn_dropout(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    double drop_prob = it ? get_node_float(*it++) : 0.5; // Default dropout probability = 0.5
    
    // Ensure drop_prob is between 0 and 1
    drop_prob = drop_prob < 0 ? 0 : (drop_prob > 1 ? 1 : drop_prob);
    
    // Return a hashmap containing the layer parameters
    hash_map_ptr_t layer = new_hash_map();
    layer->assoc_inplace(new_node_keyword("type"), new_node_keyword("dropout"), node_eq);
    layer->assoc_inplace(new_node_keyword("drop-prob"), new_node_float(drop_prob), node_eq);
    layer->assoc_inplace(new_node_keyword("scale"), new_node_float(1.0 / (1.0 - drop_prob)), node_eq);
    
    return new_node_hash_map(layer);
}

// Dropout layer forward pass
// Arguments: (layer input training)
static node_idx_t native_nn_dropout_forward(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t layer_idx = *it++;
    node_idx_t input_idx = *it++;
    bool training = it && !get_node_bool(*it++); // Default to non-training mode
    
    hash_map_ptr_t layer = get_node(layer_idx)->as_hash_map();
    double drop_prob = get_node_float(layer->get(new_node_keyword("drop-prob"), node_eq));
    double scale = get_node_float(layer->get(new_node_keyword("scale"), node_eq));
    
    // If not in training mode or dropout probability is 0, return input unchanged
    if (!training || drop_prob == 0) {
        return input_idx;
    }
    
    node_t *input = get_node(input_idx);
    
    if (input->is_matrix()) {
        matrix_ptr_t input_mat = input->as_matrix();
        matrix_ptr_t mask = new_matrix(input_mat->width, input_mat->height);
        matrix_ptr_t output = new_matrix(input_mat->width, input_mat->height);
        
        // Generate dropout mask and apply scaling
        for (int i = 0; i < input_mat->width; i++) {
            for (int j = 0; j < input_mat->height; j++) {
                double rand_val = jo_pcg32(&jo_rnd_state) / (double)UINT32_MAX;
                double mask_val = rand_val > drop_prob ? 1.0 : 0.0;
                mask->set(i, j, new_node_float(mask_val));
                
                double input_val = get_node_float(input_mat->get(i, j));
                output->set(i, j, new_node_float(input_val * mask_val * scale));
            }
        }
        
        // Store the generated mask for backward pass
        layer->assoc_inplace(new_node_keyword("last-mask"), new_node_matrix(mask), node_eq);
        
        return new_node_matrix(output);
    }
    else if (input->is_vector()) {
        vector_ptr_t input_vec = input->as_vector();
        vector_ptr_t mask = new_vector();
        vector_ptr_t output = new_vector();
        
        // Generate dropout mask and apply scaling
        for (size_t i = 0; i < input_vec->size(); i++) {
            double rand_val = jo_pcg32(&jo_rnd_state) / (double)UINT32_MAX;
            double mask_val = rand_val > drop_prob ? 1.0 : 0.0;
            mask->push_back_inplace(new_node_float(mask_val));
            
            double input_val = get_node_float(input_vec->nth(i));
            output->push_back_inplace(new_node_float(input_val * mask_val * scale));
        }
        
        // Store the generated mask for backward pass
        layer->assoc_inplace(new_node_keyword("last-mask"), new_node_vector(mask), node_eq);
        
        return new_node_vector(output);
    }
    
    warnf("nn/dropout-forward: unsupported input type\n");
    return NIL_NODE;
}

// Batch Normalization layer implementation
// Arguments: (features momentum epsilon)
static node_idx_t native_nn_batch_norm(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    int features = get_node_int(*it++); 
    double momentum = it ? get_node_float(*it++) : 0.1; // Default momentum
    double epsilon = it ? get_node_float(*it++) : 1e-5; // Default epsilon
    
    // Initialize gamma (scale) and beta (shift) parameters
    matrix_ptr_t gamma = new_matrix(features, 1);
    matrix_ptr_t beta = new_matrix(features, 1);
    
    // Initialize running mean and variance for inference
    matrix_ptr_t running_mean = new_matrix(features, 1);
    matrix_ptr_t running_var = new_matrix(features, 1);
    
    // Initialize gamma to ones and the rest to zeros
    for (int i = 0; i < features; i++) {
        gamma->set(i, 0, ONE_NODE);
        beta->set(i, 0, ZERO_NODE);
        running_mean->set(i, 0, ZERO_NODE);
        running_var->set(i, 0, ZERO_NODE);
    }
    
    // Return a hashmap containing the layer parameters
    hash_map_ptr_t layer = new_hash_map();
    layer->assoc_inplace(new_node_keyword("type"), new_node_keyword("batch-norm"), node_eq);
    layer->assoc_inplace(new_node_keyword("features"), new_node_int(features), node_eq);
    layer->assoc_inplace(new_node_keyword("momentum"), new_node_float(momentum), node_eq);
    layer->assoc_inplace(new_node_keyword("epsilon"), new_node_float(epsilon), node_eq);
    layer->assoc_inplace(new_node_keyword("gamma"), new_node_matrix(gamma), node_eq);
    layer->assoc_inplace(new_node_keyword("beta"), new_node_matrix(beta), node_eq);
    layer->assoc_inplace(new_node_keyword("running-mean"), new_node_matrix(running_mean), node_eq);
    layer->assoc_inplace(new_node_keyword("running-var"), new_node_matrix(running_var), node_eq);
    
    return new_node_hash_map(layer);
}

// Batch Normalization forward pass
// Arguments: (layer input training)
static node_idx_t native_nn_batch_norm_forward(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t layer_idx = *it++;
    node_idx_t input_idx = *it++;
    bool training = it && !get_node_bool(*it++); // Default to non-training mode
    
    hash_map_ptr_t layer = get_node(layer_idx)->as_hash_map();
    double momentum = get_node_float(layer->get(new_node_keyword("momentum"), node_eq));
    double epsilon = get_node_float(layer->get(new_node_keyword("epsilon"), node_eq));
    matrix_ptr_t gamma = get_node(layer->get(new_node_keyword("gamma"), node_eq))->as_matrix();
    matrix_ptr_t beta = get_node(layer->get(new_node_keyword("beta"), node_eq))->as_matrix();
    matrix_ptr_t running_mean = get_node(layer->get(new_node_keyword("running-mean"), node_eq))->as_matrix();
    matrix_ptr_t running_var = get_node(layer->get(new_node_keyword("running-var"), node_eq))->as_matrix();
    
    matrix_ptr_t input = get_node(input_idx)->as_matrix();
    
    // Input should be of shape (features, batch_size)
    int features = input->width;
    int batch_size = input->height;
    
    matrix_ptr_t output = new_matrix(features, batch_size);
    
    if (training) {
        // Calculate batch mean
        matrix_ptr_t batch_mean = new_matrix(features, 1);
        for (int i = 0; i < features; i++) {
            double sum = 0.0;
            for (int j = 0; j < batch_size; j++) {
                sum += get_node_float(input->get(i, j));
            }
            batch_mean->set(i, 0, new_node_float(sum / batch_size));
        }
        
        // Calculate batch variance
        matrix_ptr_t batch_var = new_matrix(features, 1);
        for (int i = 0; i < features; i++) {
            double mean = get_node_float(batch_mean->get(i, 0));
            double sum_sq_diff = 0.0;
            for (int j = 0; j < batch_size; j++) {
                double x = get_node_float(input->get(i, j));
                double diff = x - mean;
                sum_sq_diff += diff * diff;
            }
            batch_var->set(i, 0, new_node_float(sum_sq_diff / batch_size));
        }
        
        // Update running statistics
        matrix_ptr_t new_running_mean = new_matrix(features, 1);
        matrix_ptr_t new_running_var = new_matrix(features, 1);
        for (int i = 0; i < features; i++) {
            double curr_mean = get_node_float(running_mean->get(i, 0));
            double curr_var = get_node_float(running_var->get(i, 0));
            double batch_mean_val = get_node_float(batch_mean->get(i, 0));
            double batch_var_val = get_node_float(batch_var->get(i, 0));
            
            double new_mean = (1 - momentum) * curr_mean + momentum * batch_mean_val;
            double new_var = (1 - momentum) * curr_var + momentum * batch_var_val;
            
            new_running_mean->set(i, 0, new_node_float(new_mean));
            new_running_var->set(i, 0, new_node_float(new_var));
        }
        
        // Store updated running statistics in layer
        layer->assoc_inplace(new_node_keyword("running-mean"), new_node_matrix(new_running_mean), node_eq);
        layer->assoc_inplace(new_node_keyword("running-var"), new_node_matrix(new_running_var), node_eq);
        
        // Normalize, scale and shift
        for (int i = 0; i < features; i++) {
            double mean = get_node_float(batch_mean->get(i, 0));
            double var = get_node_float(batch_var->get(i, 0));
            double scale = get_node_float(gamma->get(i, 0));
            double shift = get_node_float(beta->get(i, 0));
            
            for (int j = 0; j < batch_size; j++) {
                double x = get_node_float(input->get(i, j));
                double normalized = (x - mean) / sqrt(var + epsilon);
                double result = scale * normalized + shift;
                output->set(i, j, new_node_float(result));
            }
        }
        
        // Cache values for backward pass
        layer->assoc_inplace(new_node_keyword("last-input"), input_idx, node_eq);
        layer->assoc_inplace(new_node_keyword("last-batch-mean"), new_node_matrix(batch_mean), node_eq);
        layer->assoc_inplace(new_node_keyword("last-batch-var"), new_node_matrix(batch_var), node_eq);
    } else {
        // Inference mode - use running statistics
        for (int i = 0; i < features; i++) {
            double mean = get_node_float(running_mean->get(i, 0));
            double var = get_node_float(running_var->get(i, 0));
            double scale = get_node_float(gamma->get(i, 0));
            double shift = get_node_float(beta->get(i, 0));
            
            for (int j = 0; j < batch_size; j++) {
                double x = get_node_float(input->get(i, j));
                double normalized = (x - mean) / sqrt(var + epsilon);
                double result = scale * normalized + shift;
                output->set(i, j, new_node_float(result));
            }
        }
    }
    
    return new_node_matrix(output);
}

// Conv1d layer implementation
// Arguments: (in_channels out_channels kernel_size stride padding)
static node_idx_t native_nn_conv1d(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    int in_channels = get_node_int(*it++);
    int out_channels = get_node_int(*it++);
    int kernel_size = get_node_int(*it++);
    int stride = it ? get_node_int(*it++) : 1;      // Default stride = 1
    int padding = it ? get_node_int(*it++) : 0;     // Default padding = 0
    
    // Initialize weights with shape (out_channels, in_channels, kernel_size)
    // Represented as a matrix with shape (out_channels, in_channels * kernel_size)
    matrix_ptr_t weights = new_matrix(out_channels, in_channels * kernel_size);
    
    // Initialize bias with shape (out_channels, 1)
    matrix_ptr_t bias = new_matrix(out_channels, 1);
    
    // Xavier initialization for weights
    double scale = sqrt(2.0 / (in_channels * kernel_size + out_channels));
    
    for (int oc = 0; oc < out_channels; oc++) {
        // Initialize bias to zero
        bias->set(oc, 0, ZERO_NODE);
        
        // Initialize weights with Xavier initialization
        for (int ic = 0; ic < in_channels; ic++) {
            for (int k = 0; k < kernel_size; k++) {
                int idx = ic * kernel_size + k;
                double rnd = (jo_pcg32(&jo_rnd_state) / (double)UINT32_MAX * 2.0 - 1.0) * scale;
                weights->set(oc, idx, new_node_float(rnd));
            }
        }
    }
    
    // Return a hashmap containing the layer parameters
    hash_map_ptr_t layer = new_hash_map();
    layer->assoc_inplace(new_node_keyword("type"), new_node_keyword("conv1d"), node_eq);
    layer->assoc_inplace(new_node_keyword("in-channels"), new_node_int(in_channels), node_eq);
    layer->assoc_inplace(new_node_keyword("out-channels"), new_node_int(out_channels), node_eq);
    layer->assoc_inplace(new_node_keyword("kernel-size"), new_node_int(kernel_size), node_eq);
    layer->assoc_inplace(new_node_keyword("stride"), new_node_int(stride), node_eq);
    layer->assoc_inplace(new_node_keyword("padding"), new_node_int(padding), node_eq);
    layer->assoc_inplace(new_node_keyword("weights"), new_node_matrix(weights), node_eq);
    layer->assoc_inplace(new_node_keyword("bias"), new_node_matrix(bias), node_eq);
    
    return new_node_hash_map(layer);
}

// Conv1d forward pass
// Arguments: (layer input)
// Input shape: (batch_size, in_channels, sequence_length)
// Output shape: (batch_size, out_channels, output_length)
// Where output_length = (sequence_length + 2*padding - kernel_size) / stride + 1
static node_idx_t native_nn_conv1d_forward(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t layer_idx = *it++;
    node_idx_t input_idx = *it++;
    
    hash_map_ptr_t layer = get_node(layer_idx)->as_hash_map();
    int in_channels = get_node_int(layer->get(new_node_keyword("in-channels"), node_eq));
    int out_channels = get_node_int(layer->get(new_node_keyword("out-channels"), node_eq));
    int kernel_size = get_node_int(layer->get(new_node_keyword("kernel-size"), node_eq));
    int stride = get_node_int(layer->get(new_node_keyword("stride"), node_eq));
    int padding = get_node_int(layer->get(new_node_keyword("padding"), node_eq));
    matrix_ptr_t weights = get_node(layer->get(new_node_keyword("weights"), node_eq))->as_matrix();
    matrix_ptr_t bias = get_node(layer->get(new_node_keyword("bias"), node_eq))->as_matrix();
    
    // Input is expected to be a 3D tensor represented as a hashmap
    hash_map_ptr_t input_tensor = get_node(input_idx)->as_hash_map();
    int batch_size = get_node_int(input_tensor->get(new_node_keyword("batch-size"), node_eq));
    int seq_length = get_node_int(input_tensor->get(new_node_keyword("seq-length"), node_eq));
    
    // Verify input channels match
    int input_channels = get_node_int(input_tensor->get(new_node_keyword("channels"), node_eq));
    if (input_channels != in_channels) {
        warnf("nn/conv1d-forward: input channels (%d) don't match layer's input channels (%d)\n", 
              input_channels, in_channels);
        return NIL_NODE;
    }
    
    // Get data matrix (channels*seq_length, batch_size)
    matrix_ptr_t input_data = get_node(input_tensor->get(new_node_keyword("data"), node_eq))->as_matrix();
    
    // Calculate output sequence length
    int output_length = (seq_length + 2 * padding - kernel_size) / stride + 1;
    
    // Create output data matrix (out_channels*output_length, batch_size)
    matrix_ptr_t output_data = new_matrix(out_channels * output_length, batch_size);
    
    // For each batch and output position
    for (int b = 0; b < batch_size; b++) {
        for (int oc = 0; oc < out_channels; oc++) {
            // Get bias for this output channel
            double bias_val = get_node_float(bias->get(oc, 0));
            
            for (int out_pos = 0; out_pos < output_length; out_pos++) {
                // Calculate the position in the input sequence
                int in_start = out_pos * stride - padding;
                
                // Initialize with bias
                double result = bias_val;
                
                // Convolve over input channels and kernel positions
                for (int ic = 0; ic < in_channels; ic++) {
                    for (int k = 0; k < kernel_size; k++) {
                        int in_pos = in_start + k;
                        
                        // Skip if out of bounds (implicit zero padding)
                        if (in_pos < 0 || in_pos >= seq_length) continue;
                        
                        // Get input value and weight
                        int in_idx = ic * seq_length + in_pos;
                        int weight_idx = ic * kernel_size + k;
                        
                        double in_val = get_node_float(input_data->get(in_idx, b));
                        double weight_val = get_node_float(weights->get(oc, weight_idx));
                        
                        result += in_val * weight_val;
                    }
                }
                
                // Store the result
                int out_idx = oc * output_length + out_pos;
                output_data->set(out_idx, b, new_node_float(result));
            }
        }
    }
    
    // Create output tensor
    hash_map_ptr_t output_tensor = new_hash_map();
    output_tensor->assoc_inplace(new_node_keyword("batch-size"), new_node_int(batch_size), node_eq);
    output_tensor->assoc_inplace(new_node_keyword("channels"), new_node_int(out_channels), node_eq);
    output_tensor->assoc_inplace(new_node_keyword("seq-length"), new_node_int(output_length), node_eq);
    output_tensor->assoc_inplace(new_node_keyword("data"), new_node_matrix(output_data), node_eq);
    
    // Cache input for backward pass
    hash_map_ptr_t updated_layer = new_hash_map(*layer);
    updated_layer->assoc_inplace(new_node_keyword("last-input"), input_idx, node_eq);
    
    return new_node_hash_map(output_tensor);
}

// MaxPool1d layer implementation
// Arguments: (kernel_size stride padding)
static node_idx_t native_nn_max_pool1d(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    int kernel_size = get_node_int(*it++);
    int stride = it ? get_node_int(*it++) : kernel_size; // Default stride = kernel_size
    int padding = it ? get_node_int(*it++) : 0;          // Default padding = 0
    
    // Return a hashmap containing the layer parameters
    hash_map_ptr_t layer = new_hash_map();
    layer->assoc_inplace(new_node_keyword("type"), new_node_keyword("max-pool1d"), node_eq);
    layer->assoc_inplace(new_node_keyword("kernel-size"), new_node_int(kernel_size), node_eq);
    layer->assoc_inplace(new_node_keyword("stride"), new_node_int(stride), node_eq);
    layer->assoc_inplace(new_node_keyword("padding"), new_node_int(padding), node_eq);
    
    return new_node_hash_map(layer);
}

// MaxPool1d forward pass
// Arguments: (layer input)
static node_idx_t native_nn_max_pool1d_forward(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t layer_idx = *it++;
    node_idx_t input_idx = *it++;
    
    hash_map_ptr_t layer = get_node(layer_idx)->as_hash_map();
    int kernel_size = get_node_int(layer->get(new_node_keyword("kernel-size"), node_eq));
    int stride = get_node_int(layer->get(new_node_keyword("stride"), node_eq));
    int padding = get_node_int(layer->get(new_node_keyword("padding"), node_eq));
    
    // Input is expected to be a 3D tensor represented as a hashmap
    hash_map_ptr_t input_tensor = get_node(input_idx)->as_hash_map();
    int batch_size = get_node_int(input_tensor->get(new_node_keyword("batch-size"), node_eq));
    int channels = get_node_int(input_tensor->get(new_node_keyword("channels"), node_eq));
    int seq_length = get_node_int(input_tensor->get(new_node_keyword("seq-length"), node_eq));
    
    // Get data matrix (channels*seq_length, batch_size)
    matrix_ptr_t input_data = get_node(input_tensor->get(new_node_keyword("data"), node_eq))->as_matrix();
    
    // Calculate output sequence length
    int output_length = (seq_length + 2 * padding - kernel_size) / stride + 1;
    
    // Create output data matrix (channels*output_length, batch_size)
    matrix_ptr_t output_data = new_matrix(channels * output_length, batch_size);
    
    // Create indices matrix to store the position of max values (for backward pass)
    matrix_ptr_t indices = new_matrix(channels * output_length, batch_size);
    
    // For each batch and channel
    for (int b = 0; b < batch_size; b++) {
        for (int c = 0; c < channels; c++) {
            for (int out_pos = 0; out_pos < output_length; out_pos++) {
                // Calculate the position in the input sequence
                int in_start = out_pos * stride - padding;
                
                // Find maximum value in the window
                double max_val = -INFINITY;
                int max_idx = -1;
                
                for (int k = 0; k < kernel_size; k++) {
                    int in_pos = in_start + k;
                    
                    // Skip if out of bounds (implicit zero padding)
                    if (in_pos < 0 || in_pos >= seq_length) continue;
                    
                    // Get input value
                    int in_idx = c * seq_length + in_pos;
                    double val = get_node_float(input_data->get(in_idx, b));
                    
                    // Update max if needed
                    if (val > max_val) {
                        max_val = val;
                        max_idx = in_pos;
                    }
                }
                
                // Store the maximum value and its index
                int out_idx = c * output_length + out_pos;
                output_data->set(out_idx, b, new_node_float(max_val));
                indices->set(out_idx, b, new_node_int(max_idx));
            }
        }
    }
    
    // Create output tensor
    hash_map_ptr_t output_tensor = new_hash_map();
    output_tensor->assoc_inplace(new_node_keyword("batch-size"), new_node_int(batch_size), node_eq);
    output_tensor->assoc_inplace(new_node_keyword("channels"), new_node_int(channels), node_eq);
    output_tensor->assoc_inplace(new_node_keyword("seq-length"), new_node_int(output_length), node_eq);
    output_tensor->assoc_inplace(new_node_keyword("data"), new_node_matrix(output_data), node_eq);
    
    // Cache indices and input for backward pass
    hash_map_ptr_t updated_layer = new_hash_map(*layer);
    updated_layer->assoc_inplace(new_node_keyword("last-input"), input_idx, node_eq);
    updated_layer->assoc_inplace(new_node_keyword("indices"), new_node_matrix(indices), node_eq);
    
    return new_node_hash_map(output_tensor);
} 

// RMSprop Optimizer
// Arguments: (model learning_rate alpha epsilon)
static node_idx_t native_nn_rmsprop(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t model_idx = *it++;
    node_idx_t lr_idx = *it++;
    
    double alpha = 0.99;  // Decay rate, default = 0.99
    double epsilon = 1e-8; // Small constant for numerical stability
    
    if (it) alpha = get_node_float(*it++);
    if (it) epsilon = get_node_float(*it++);
    
    hash_map_ptr_t optimizer = new_hash_map();
    optimizer->assoc_inplace(new_node_keyword("type"), new_node_keyword("rmsprop"), node_eq);
    optimizer->assoc_inplace(new_node_keyword("learning-rate"), lr_idx, node_eq);
    optimizer->assoc_inplace(new_node_keyword("alpha"), new_node_float(alpha), node_eq);
    optimizer->assoc_inplace(new_node_keyword("epsilon"), new_node_float(epsilon), node_eq);
    optimizer->assoc_inplace(new_node_keyword("model"), model_idx, node_eq);
    
    // Initialize squared gradient accumulators
    hash_map_ptr_t v = new_hash_map();
    
    hash_map_ptr_t model = get_node(model_idx)->as_hash_map();
    // Iterate through layers
    for (hash_map_t::iterator layer_it = model->begin(); layer_it; layer_it++) {
        node_idx_t layer_key = layer_it->first;
        node_t* layer_node = get_node(layer_it->second);
        
        if (layer_node->is_hash_map()) {
             hash_map_ptr_t layer_params = layer_node->as_hash_map();
             hash_map_ptr_t layer_v = new_hash_map();

             // Iterate through params within the layer
             for (hash_map_t::iterator param_it = layer_params->begin(); param_it; param_it++) {
                 node_idx_t param_key = param_it->first;
                 node_t* param_node = get_node(param_it->second);
                 
                 if (param_node->is_matrix()) {
                     matrix_ptr_t param_mat = param_node->as_matrix();
                     matrix_ptr_t v_mat = new_matrix(param_mat->width, param_mat->height);

                     // Initialize squared gradient accumulator to zero
                     for (int i = 0; i < param_mat->width; i++) {
                         for (int j = 0; j < param_mat->height; j++) {
                             v_mat->set(i, j, ZERO_NODE);
                         }
                     }
                     layer_v->assoc_inplace(param_key, new_node_matrix(v_mat), node_eq);
                 }
             }
             v->assoc_inplace(layer_key, new_node_hash_map(layer_v), node_eq);
        }
    }
    
    optimizer->assoc_inplace(new_node_keyword("v"), new_node_hash_map(v), node_eq);
    
    return new_node_hash_map(optimizer);
}

// RMSprop step - Update model parameters
// Arguments: (optimizer gradients)
static node_idx_t native_nn_rmsprop_step(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t optimizer_idx = *it++;
    node_idx_t gradients_idx = *it++;
    
    hash_map_ptr_t optimizer = get_node(optimizer_idx)->as_hash_map();
    hash_map_ptr_t gradients = get_node(gradients_idx)->as_hash_map();
    node_idx_t model_idx = optimizer->get(new_node_keyword("model"), node_eq);
    double lr = get_node_float(optimizer->get(new_node_keyword("learning-rate"), node_eq));
    double alpha = get_node_float(optimizer->get(new_node_keyword("alpha"), node_eq));
    double epsilon = get_node_float(optimizer->get(new_node_keyword("epsilon"), node_eq));
    
    // Squared gradient accumulators
    hash_map_ptr_t v = get_node(optimizer->get(new_node_keyword("v"), node_eq))->as_hash_map();
    
    // Create copies for the updated state
    hash_map_ptr_t model = new_hash_map(*get_node(model_idx)->as_hash_map());
    hash_map_ptr_t v_new = new_hash_map();
    
    // Iterate through layers in the model
    for (hash_map_t::iterator layer_it = model->begin(); layer_it; layer_it++) {
        node_idx_t layer_key = layer_it->first;
        node_idx_t layer_node_idx = layer_it->second;
        node_t* layer_node = get_node(layer_node_idx);

        if (!layer_node->is_hash_map()) continue;
        hash_map_ptr_t layer_params = layer_node->as_hash_map();
        
        // Get the corresponding gradients and v for this layer
        node_idx_t layer_gradients_idx = gradients->get(layer_key, node_eq);
        node_idx_t layer_v_idx = v->get(layer_key, node_eq);

        // Check if gradients/accumulators exist for this layer
        if (layer_gradients_idx == INV_NODE || layer_v_idx == INV_NODE) continue;
        
        hash_map_ptr_t layer_gradients = get_node(layer_gradients_idx)->as_hash_map();
        hash_map_ptr_t layer_v = get_node(layer_v_idx)->as_hash_map();

        // Create maps to hold updated params and accumulators for this layer
        hash_map_ptr_t updated_layer_params = new_hash_map(*layer_params);
        hash_map_ptr_t updated_layer_v = new_hash_map();

        // Iterate through parameters within the layer
        for (hash_map_t::iterator param_it = layer_params->begin(); param_it; param_it++) {
            node_idx_t param_key = param_it->first;
            node_idx_t param = param_it->second;
            
            // Look up gradient and accumulator
            node_idx_t grad = layer_gradients->get(param_key, node_eq);
            node_idx_t v_param = layer_v->get(param_key, node_eq);

            // Skip if no gradient or accumulator found
            if (grad == INV_NODE || v_param == INV_NODE) continue;
            
            node_t *param_node = get_node(param);
            node_t *grad_node = get_node(grad);
            node_t *v_node = get_node(v_param);
            
            if (param_node->is_matrix() && grad_node->is_matrix() && v_node->is_matrix()) {
                matrix_ptr_t param_mat = param_node->as_matrix();
                matrix_ptr_t grad_mat = grad_node->as_matrix();
                matrix_ptr_t v_mat = v_node->as_matrix();
                
                matrix_ptr_t v_new_mat = new_matrix(v_mat->width, v_mat->height);
                matrix_ptr_t updated_param_mat = new_matrix(param_mat->width, param_mat->height);
                
                for (int i = 0; i < param_mat->width; i++) {
                    for (int j = 0; j < param_mat->height; j++) {
                        double p = get_node_float(param_mat->get(i, j));
                        double g = get_node_float(grad_mat->get(i, j));
                        double v_val = get_node_float(v_mat->get(i, j));
                        
                        // Update squared gradient accumulator
                        double v_new_val = alpha * v_val + (1 - alpha) * g * g;
                        v_new_mat->set(i, j, new_node_float(v_new_val));
                        
                        // Update parameter
                        double param_new = p - lr * g / (sqrt(v_new_val) + epsilon);
                        updated_param_mat->set(i, j, new_node_float(param_new));
                    }
                }
                
                // Update the parameter within the layer's updated map
                updated_layer_params->assoc_inplace(param_key, new_node_matrix(updated_param_mat), node_eq);
                // Store the new accumulator in the layer's updated map
                updated_layer_v->assoc_inplace(param_key, new_node_matrix(v_new_mat), node_eq);
            }
        }
        // Update the layer in the main model map
        model->assoc_inplace(layer_key, new_node_hash_map(updated_layer_params), node_eq);
        // Store the updated accumulators for the layer
        v_new->assoc_inplace(layer_key, new_node_hash_map(updated_layer_v), node_eq);
    }
    
    // Update optimizer state
    hash_map_ptr_t updated_optimizer = new_hash_map(*optimizer);
    updated_optimizer->assoc_inplace(new_node_keyword("model"), new_node_hash_map(model), node_eq);
    updated_optimizer->assoc_inplace(new_node_keyword("v"), new_node_hash_map(v_new), node_eq);
    
    return new_node_hash_map(updated_optimizer);
}

// AdamW Optimizer (Adam with decoupled weight decay)
// Arguments: (model learning_rate weight_decay beta1 beta2 epsilon)
static node_idx_t native_nn_adamw(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t model_idx = *it++;
    node_idx_t lr_idx = *it++;
    
    double weight_decay = 0.01; // Default weight decay
    double beta1 = 0.9, beta2 = 0.999, epsilon = 1e-8;
    
    if (it) weight_decay = get_node_float(*it++);
    if (it) beta1 = get_node_float(*it++);
    if (it) beta2 = get_node_float(*it++);
    if (it) epsilon = get_node_float(*it++);
    
    hash_map_ptr_t optimizer = new_hash_map();
    optimizer->assoc_inplace(new_node_keyword("type"), new_node_keyword("adamw"), node_eq);
    optimizer->assoc_inplace(new_node_keyword("learning-rate"), lr_idx, node_eq);
    optimizer->assoc_inplace(new_node_keyword("weight-decay"), new_node_float(weight_decay), node_eq);
    optimizer->assoc_inplace(new_node_keyword("beta1"), new_node_float(beta1), node_eq);
    optimizer->assoc_inplace(new_node_keyword("beta2"), new_node_float(beta2), node_eq);
    optimizer->assoc_inplace(new_node_keyword("epsilon"), new_node_float(epsilon), node_eq);
    optimizer->assoc_inplace(new_node_keyword("model"), model_idx, node_eq);
    optimizer->assoc_inplace(new_node_keyword("t"), ONE_NODE, node_eq);
    
    // Initialize moment estimates (m and v) with nested structure
    hash_map_ptr_t m = new_hash_map();
    hash_map_ptr_t v = new_hash_map();
    
    hash_map_ptr_t model = get_node(model_idx)->as_hash_map();
    // Iterate through layers
    for (hash_map_t::iterator layer_it = model->begin(); layer_it; layer_it++) {
        node_idx_t layer_key = layer_it->first;
        node_t* layer_node = get_node(layer_it->second);
        
        if (layer_node->is_hash_map()) {
             hash_map_ptr_t layer_params = layer_node->as_hash_map();
             hash_map_ptr_t layer_m = new_hash_map();
             hash_map_ptr_t layer_v = new_hash_map();

             // Iterate through params within the layer
             for (hash_map_t::iterator param_it = layer_params->begin(); param_it; param_it++) {
                 node_idx_t param_key = param_it->first;
                 node_t* param_node = get_node(param_it->second);
                 
                 if (param_node->is_matrix()) {
                     matrix_ptr_t param_mat = param_node->as_matrix();
                     matrix_ptr_t m_mat = new_matrix(param_mat->width, param_mat->height);
                     matrix_ptr_t v_mat = new_matrix(param_mat->width, param_mat->height);

                     // Initialize moment matrices to zero
                     for (int i = 0; i < param_mat->width; i++) {
                         for (int j = 0; j < param_mat->height; j++) {
                             m_mat->set(i, j, ZERO_NODE);
                             v_mat->set(i, j, ZERO_NODE);
                         }
                     }
                     layer_m->assoc_inplace(param_key, new_node_matrix(m_mat), node_eq);
                     layer_v->assoc_inplace(param_key, new_node_matrix(v_mat), node_eq);
                 }
             }
             m->assoc_inplace(layer_key, new_node_hash_map(layer_m), node_eq);
             v->assoc_inplace(layer_key, new_node_hash_map(layer_v), node_eq);
        }
    }
    
    optimizer->assoc_inplace(new_node_keyword("m"), new_node_hash_map(m), node_eq);
    optimizer->assoc_inplace(new_node_keyword("v"), new_node_hash_map(v), node_eq);
    
    return new_node_hash_map(optimizer);
}

// AdamW step - Update model parameters with decoupled weight decay
// Arguments: (optimizer gradients)
static node_idx_t native_nn_adamw_step(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t optimizer_idx = *it++;
    node_idx_t gradients_idx = *it++;
    
    hash_map_ptr_t optimizer = get_node(optimizer_idx)->as_hash_map();
    hash_map_ptr_t gradients = get_node(gradients_idx)->as_hash_map(); 
    node_idx_t model_idx = optimizer->get(new_node_keyword("model"), node_eq);
    double lr = get_node_float(optimizer->get(new_node_keyword("learning-rate"), node_eq));
    double weight_decay = get_node_float(optimizer->get(new_node_keyword("weight-decay"), node_eq));
    double beta1 = get_node_float(optimizer->get(new_node_keyword("beta1"), node_eq));
    double beta2 = get_node_float(optimizer->get(new_node_keyword("beta2"), node_eq));
    double epsilon = get_node_float(optimizer->get(new_node_keyword("epsilon"), node_eq));
    int t = get_node_int(optimizer->get(new_node_keyword("t"), node_eq));
    
    // Moment estimates
    hash_map_ptr_t m = get_node(optimizer->get(new_node_keyword("m"), node_eq))->as_hash_map();
    hash_map_ptr_t v = get_node(optimizer->get(new_node_keyword("v"), node_eq))->as_hash_map();
    
    // Create copies for the updated state
    hash_map_ptr_t model = new_hash_map(*get_node(model_idx)->as_hash_map());
    hash_map_ptr_t m_new = new_hash_map();
    hash_map_ptr_t v_new = new_hash_map();
    
    // Iterate through layers in the model
    for (hash_map_t::iterator layer_it = model->begin(); layer_it; layer_it++) {
        node_idx_t layer_key = layer_it->first;
        node_idx_t layer_node_idx = layer_it->second;
        node_t* layer_node = get_node(layer_node_idx);

        if (!layer_node->is_hash_map()) continue;
        hash_map_ptr_t layer_params = layer_node->as_hash_map();
        
        // Get the corresponding gradients, m, and v for this layer
        node_idx_t layer_gradients_idx = gradients->get(layer_key, node_eq);
        node_idx_t layer_m_idx = m->get(layer_key, node_eq);
        node_idx_t layer_v_idx = v->get(layer_key, node_eq);

        // Check if gradients/moments exist for this layer
        if (layer_gradients_idx == INV_NODE || layer_m_idx == INV_NODE || layer_v_idx == INV_NODE) continue;
        
        hash_map_ptr_t layer_gradients = get_node(layer_gradients_idx)->as_hash_map();
        hash_map_ptr_t layer_m = get_node(layer_m_idx)->as_hash_map();
        hash_map_ptr_t layer_v = get_node(layer_v_idx)->as_hash_map();

        // Create maps to hold updated params and moments for this layer
        hash_map_ptr_t updated_layer_params = new_hash_map(*layer_params); 
        hash_map_ptr_t updated_layer_m = new_hash_map();
        hash_map_ptr_t updated_layer_v = new_hash_map();

        // Iterate through parameters within the layer
        for (hash_map_t::iterator param_it = layer_params->begin(); param_it; param_it++) {
            node_idx_t param_key = param_it->first; 
            node_idx_t param = param_it->second;
            
            // Look up gradient and moments
            node_idx_t grad = layer_gradients->get(param_key, node_eq);
            node_idx_t m_param = layer_m->get(param_key, node_eq);
            node_idx_t v_param = layer_v->get(param_key, node_eq);

            // Skip if no gradient or moments found
            if (grad == INV_NODE || m_param == INV_NODE || v_param == INV_NODE) continue;
            
            node_t *param_node = get_node(param);
            node_t *grad_node = get_node(grad);
            node_t *m_node = get_node(m_param);
            node_t *v_node = get_node(v_param);
            
            if (param_node->is_matrix() && grad_node->is_matrix() && 
                m_node->is_matrix() && v_node->is_matrix()) {
                
                matrix_ptr_t param_mat = param_node->as_matrix();
                matrix_ptr_t grad_mat = grad_node->as_matrix();
                matrix_ptr_t m_mat = m_node->as_matrix();
                matrix_ptr_t v_mat = v_node->as_matrix();
                
                matrix_ptr_t m_new_mat = new_matrix(m_mat->width, m_mat->height);
                matrix_ptr_t v_new_mat = new_matrix(v_mat->width, v_mat->height);
                matrix_ptr_t updated_param_mat = new_matrix(param_mat->width, param_mat->height);
                
                for (int i = 0; i < param_mat->width; i++) {
                    for (int j = 0; j < param_mat->height; j++) {
                        double p = get_node_float(param_mat->get(i, j));
                        double g = get_node_float(grad_mat->get(i, j));
                        double m_val = get_node_float(m_mat->get(i, j));
                        double v_val = get_node_float(v_mat->get(i, j));
                        
                        // Update biased first moment estimate
                        double m_new_val = beta1 * m_val + (1 - beta1) * g;
                        m_new_mat->set(i, j, new_node_float(m_new_val));
                        
                        // Update biased second raw moment estimate
                        double v_new_val = beta2 * v_val + (1 - beta2) * g * g;
                        v_new_mat->set(i, j, new_node_float(v_new_val));
                        
                        // Bias correction
                        double m_hat = m_new_val / (1 - pow(beta1, t));
                        double v_hat = v_new_val / (1 - pow(beta2, t));
                        
                        // Decoupled weight decay
                        double weight_decay_term = weight_decay * p;
                        
                        // Update parameter with weight decay
                        double param_new = p - lr * (m_hat / (sqrt(v_hat) + epsilon) + weight_decay_term);
                        updated_param_mat->set(i, j, new_node_float(param_new));
                    }
                }
                
                // Update the parameter within the layer's updated map
                updated_layer_params->assoc_inplace(param_key, new_node_matrix(updated_param_mat), node_eq);
                // Store the new moments in the layer's updated moment maps
                updated_layer_m->assoc_inplace(param_key, new_node_matrix(m_new_mat), node_eq);
                updated_layer_v->assoc_inplace(param_key, new_node_matrix(v_new_mat), node_eq);
            }
        }
        // Update the layer in the main model map
        model->assoc_inplace(layer_key, new_node_hash_map(updated_layer_params), node_eq);
        // Store the updated moments for the layer
        m_new->assoc_inplace(layer_key, new_node_hash_map(updated_layer_m), node_eq);
        v_new->assoc_inplace(layer_key, new_node_hash_map(updated_layer_v), node_eq);
    }
    
    // Update optimizer state
    hash_map_ptr_t updated_optimizer = new_hash_map(*optimizer);
    updated_optimizer->assoc_inplace(new_node_keyword("model"), new_node_hash_map(model), node_eq);
    updated_optimizer->assoc_inplace(new_node_keyword("m"), new_node_hash_map(m_new), node_eq);
    updated_optimizer->assoc_inplace(new_node_keyword("v"), new_node_hash_map(v_new), node_eq);
    updated_optimizer->assoc_inplace(new_node_keyword("t"), new_node_int(t + 1), node_eq);
    
    return new_node_hash_map(updated_optimizer);
}

// Create learning rate scheduler (Step decay)
// Arguments: (optimizer step_size gamma)
static node_idx_t native_nn_step_lr_scheduler(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t optimizer_idx = *it++;
    int step_size = get_node_int(*it++);
    double gamma = get_node_float(*it++);
    
    hash_map_ptr_t scheduler = new_hash_map();
    scheduler->assoc_inplace(new_node_keyword("type"), new_node_keyword("step-lr"), node_eq);
    scheduler->assoc_inplace(new_node_keyword("optimizer"), optimizer_idx, node_eq);
    scheduler->assoc_inplace(new_node_keyword("step-size"), new_node_int(step_size), node_eq);
    scheduler->assoc_inplace(new_node_keyword("gamma"), new_node_float(gamma), node_eq);
    scheduler->assoc_inplace(new_node_keyword("last-epoch"), ZERO_NODE, node_eq);
    
    // Store initial learning rate
    hash_map_ptr_t optimizer = get_node(optimizer_idx)->as_hash_map();
    node_idx_t initial_lr = optimizer->get(new_node_keyword("learning-rate"), node_eq);
    scheduler->assoc_inplace(new_node_keyword("initial-lr"), initial_lr, node_eq);
    
    return new_node_hash_map(scheduler);
}

// Step the learning rate scheduler
// Arguments: (scheduler)
static node_idx_t native_nn_scheduler_step(env_ptr_t env, list_ptr_t args) {
    node_idx_t scheduler_idx = args->first_value();
    hash_map_ptr_t scheduler = get_node(scheduler_idx)->as_hash_map();
    
    node_idx_t type_idx = scheduler->get(new_node_keyword("type"), node_eq);
    const char* scheduler_type = get_node(type_idx)->t_string.c_str();
    
    node_idx_t optimizer_idx = scheduler->get(new_node_keyword("optimizer"), node_eq);
    hash_map_ptr_t optimizer = get_node(optimizer_idx)->as_hash_map();
    
    int last_epoch = get_node_int(scheduler->get(new_node_keyword("last-epoch"), node_eq));
    int next_epoch = last_epoch + 1;
    
    hash_map_ptr_t updated_scheduler = new_hash_map(*scheduler);
    hash_map_ptr_t updated_optimizer = new_hash_map(*optimizer);
    
    if (strcmp(scheduler_type, "step-lr") == 0) {
        int step_size = get_node_int(scheduler->get(new_node_keyword("step-size"), node_eq));
        double gamma = get_node_float(scheduler->get(new_node_keyword("gamma"), node_eq));
        double initial_lr = get_node_float(scheduler->get(new_node_keyword("initial-lr"), node_eq));
        
        // Calculate new learning rate
        int steps = next_epoch / step_size;
        double new_lr = initial_lr * pow(gamma, steps);
        
        // Update optimizer with new learning rate
        updated_optimizer->assoc_inplace(new_node_keyword("learning-rate"), new_node_float(new_lr), node_eq);
    }
    // Could add more scheduler types here
    
    // Update epoch counter
    updated_scheduler->assoc_inplace(new_node_keyword("last-epoch"), new_node_int(next_epoch), node_eq);
    // Update optimizer reference
    updated_scheduler->assoc_inplace(new_node_keyword("optimizer"), new_node_hash_map(updated_optimizer), node_eq);
    
    return new_node_hash_map(updated_scheduler);
}

// Save model to file
// Arguments: (model file_path)
static node_idx_t native_nn_save_model(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t model_idx = *it++;
    const char* file_path = get_node(*it++)->t_string.c_str();
    
    hash_map_ptr_t model = get_node(model_idx)->as_hash_map();
    
    // Open file for writing
    FILE* file = fopen(file_path, "w");
    if (!file) {
        warnf("nn/save-model: failed to open file for writing: %s\n", file_path);
        return FALSE_NODE;
    }
    
    // Serialize model structure
    for (hash_map_t::iterator layer_it = model->begin(); layer_it; layer_it++) {
        node_idx_t layer_key = layer_it->first;
        const char* layer_name = get_node(layer_key)->t_string.c_str();
        
        fprintf(file, "LAYER %s\n", layer_name);
        
        node_t* layer_node = get_node(layer_it->second);
        if (!layer_node->is_hash_map()) continue;
        
        hash_map_ptr_t layer = layer_node->as_hash_map();
        
        // Write layer type and parameters
        for (hash_map_t::iterator param_it = layer->begin(); param_it; param_it++) {
            node_idx_t param_key = param_it->first;
            node_idx_t param_value = param_it->second;
            
            const char* param_name = get_node(param_key)->t_string.c_str();
            node_t* param_node = get_node(param_value);
            
            if (param_node->is_int()) {
                fprintf(file, "PARAM %s INT %d\n", param_name, param_node->t_int);
            }
            else if (param_node->is_float()) {
                fprintf(file, "PARAM %s FLOAT %f\n", param_name, param_node->t_float);
            }
            else if (param_node->is_keyword()) {
                fprintf(file, "PARAM %s KEYWORD %s\n", param_name, param_node->t_string.c_str());
            }
            else if (param_node->is_matrix()) {
                matrix_ptr_t matrix = param_node->as_matrix();
                fprintf(file, "MATRIX %s %d %d\n", param_name, matrix->width, matrix->height);
                
                // Write matrix data
                for (int i = 0; i < matrix->width; i++) {
                    for (int j = 0; j < matrix->height; j++) {
                        double value = get_node_float(matrix->get(i, j));
                        fprintf(file, "%f ", value);
                    }
                    fprintf(file, "\n");
                }
            }
        }
        
        fprintf(file, "END_LAYER\n");
    }
    
    fclose(file);
    return TRUE_NODE;
}

// Load model from file
// Arguments: (file_path)
static node_idx_t native_nn_load_model(env_ptr_t env, list_ptr_t args) {
    const char* file_path = get_node(args->first_value())->t_string.c_str();
    
    // Open file for reading
    FILE* file = fopen(file_path, "r");
    if (!file) {
        warnf("nn/load-model: failed to open file for reading: %s\n", file_path);
        return NIL_NODE;
    }
    
    hash_map_ptr_t model = new_hash_map();
    hash_map_ptr_t current_layer = NULL;
    char layer_name[256];
    
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // Skip empty lines
        if (strlen(line) == 0) continue;
        
        char cmd[64];
        if (sscanf(line, "%s", cmd) != 1) continue;
        
        if (strcmp(cmd, "LAYER") == 0) {
            // Start a new layer
            if (sscanf(line, "LAYER %s", layer_name) != 1) continue;
            current_layer = new_hash_map();
        }
        else if (strcmp(cmd, "END_LAYER") == 0) {
            // End current layer and add to model
            model->assoc_inplace(new_node_keyword(layer_name), new_node_hash_map(current_layer), node_eq);
            current_layer = NULL;
        }
        else if (strcmp(cmd, "PARAM") == 0 && current_layer) {
            char param_name[256];
            char param_type[64];
            
            if (sscanf(line, "PARAM %s %s", param_name, param_type) != 2) continue;
            
            if (strcmp(param_type, "INT") == 0) {
                int value;
                if (sscanf(line, "PARAM %s INT %d", param_name, &value) != 2) continue;
                current_layer->assoc_inplace(new_node_keyword(param_name), new_node_int(value), node_eq);
            }
            else if (strcmp(param_type, "FLOAT") == 0) {
                double value;
                if (sscanf(line, "PARAM %s FLOAT %lf", param_name, &value) != 2) continue;
                current_layer->assoc_inplace(new_node_keyword(param_name), new_node_float(value), node_eq);
            }
            else if (strcmp(param_type, "KEYWORD") == 0) {
                char value[256];
                if (sscanf(line, "PARAM %s KEYWORD %s", param_name, value) != 2) continue;
                current_layer->assoc_inplace(new_node_keyword(param_name), new_node_keyword(value), node_eq);
            }
        }
        else if (strcmp(cmd, "MATRIX") == 0 && current_layer) {
            char matrix_name[256];
            int width, height;
            
            if (sscanf(line, "MATRIX %s %d %d", matrix_name, &width, &height) != 3) continue;
            
            matrix_ptr_t matrix = new_matrix(width, height);
            
            // Read matrix data
            for (int i = 0; i < width; i++) {
                if (!fgets(line, sizeof(line), file)) {
                    warnf("nn/load-model: unexpected end of file while reading matrix\n");
                    fclose(file);
                    return NIL_NODE;
                }
                
                char* token = strtok(line, " ");
                for (int j = 0; j < height; j++) {
                    if (!token) {
                        warnf("nn/load-model: unexpected end of matrix row\n");
                        break;
                    }
                    
                    double value = atof(token);
                    matrix->set(i, j, new_node_float(value));
                    
                    token = strtok(NULL, " ");
                }
            }
            
            current_layer->assoc_inplace(new_node_keyword(matrix_name), new_node_matrix(matrix), node_eq);
        }
    }
    
    fclose(file);
    return new_node_hash_map(model);
}

// Calculate accuracy for classification
// Arguments: (predictions targets)
static node_idx_t native_nn_accuracy(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t pred_idx = *it++;
    node_idx_t targets_idx = *it++;
    
    node_t *pred = get_node(pred_idx);
    node_t *targets = get_node(targets_idx);
    
    if (pred->is_matrix() && targets->is_matrix()) {
        matrix_ptr_t pred_mat = pred->as_matrix();
        matrix_ptr_t targets_mat = targets->as_matrix();
        
        // Check dimensions
        if (pred_mat->width != targets_mat->width || pred_mat->height != targets_mat->height) {
            warnf("nn/accuracy: prediction and target dimensions don't match\n");
            return NIL_NODE;
        }
        
        int correct = 0;
        int total = 0;
        
        // For each sample (column)
        for (int j = 0; j < pred_mat->height; j++) {
            // Find the index of max value in prediction and target
            int pred_max_idx = 0;
            int target_max_idx = 0;
            double pred_max_val = get_node_float(pred_mat->get(0, j));
            double target_max_val = get_node_float(targets_mat->get(0, j));
            
            for (int i = 1; i < pred_mat->width; i++) {
                double pred_val = get_node_float(pred_mat->get(i, j));
                double target_val = get_node_float(targets_mat->get(i, j));
                
                if (pred_val > pred_max_val) {
                    pred_max_val = pred_val;
                    pred_max_idx = i;
                }
                
                if (target_val > target_max_val) {
                    target_max_val = target_val;
                    target_max_idx = i;
                }
            }
            
            // Check if prediction matches target
            if (pred_max_idx == target_max_idx) {
                correct++;
            }
            
            total++;
        }
        
        return new_node_float((double)correct / total);
    }
    else if (pred->is_vector() && targets->is_vector()) {
        vector_ptr_t pred_vec = pred->as_vector();
        vector_ptr_t targets_vec = targets->as_vector();
        
        // Check dimensions
        if (pred_vec->size() != targets_vec->size()) {
            warnf("nn/accuracy: prediction and target dimensions don't match\n");
            return NIL_NODE;
        }
        
        int correct = 0;
        int total = 0;
        
        for (size_t i = 0; i < pred_vec->size(); i++) {
            double pred_val = get_node_float(pred_vec->nth(i));
            double target_val = get_node_float(targets_vec->nth(i));
            
            // For binary classification, threshold at 0.5
            bool pred_class = pred_val >= 0.5;
            bool target_class = target_val >= 0.5;
            
            if (pred_class == target_class) {
                correct++;
            }
            
            total++;
        }
        
        return new_node_float((double)correct / total);
    }
    
    warnf("nn/accuracy: unsupported input types\n");
    return NIL_NODE;
}

// Create sequential model from layers
// Arguments: (layer1 layer2 ...)
static node_idx_t native_nn_sequential(env_ptr_t env, list_ptr_t args) {
    hash_map_ptr_t model = new_hash_map();
    
    // Process each layer
    int layer_idx = 1;
    for (list_t::iterator it(args); it; it++) {
        char layer_name[32];
        sprintf(layer_name, "layer%d", layer_idx++);
        
        model->assoc_inplace(new_node_keyword(layer_name), *it, node_eq);
    }
    
    return new_node_hash_map(model);
}

// Forward pass through sequential model
// Arguments: (model input)
static node_idx_t native_nn_sequential_forward(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t model_idx = *it++;
    node_idx_t input_idx = *it++;
    
    hash_map_ptr_t model = get_node(model_idx)->as_hash_map();
    node_idx_t current_output = input_idx;
    
    // Detect network layers and structure
    vector_ptr_t layer_keys = new_vector();
    
    // Find all layer keys and sort them
    for (hash_map_t::iterator model_it = model->begin(); model_it; model_it++) {
        node_idx_t key = model_it->first;
        node_t* key_node = get_node(key);
        
        // Only process layers that are named in the format "layerX" where X is a number
        if (key_node->type == NODE_KEYWORD) {
            const char* key_str = key_node->t_string.c_str();
            if (strncmp(key_str, "layer", 5) == 0) {
                layer_keys->push_back_inplace(key);
            }
        }
    }
    
    // Sort layer keys in ascending order (simple bubble sort)
    for (int i = 0; i < layer_keys->size(); i++) {
        for (int j = i + 1; j < layer_keys->size(); j++) {
            const char* key_i = get_node(layer_keys->nth(i))->t_string.c_str();
            const char* key_j = get_node(layer_keys->nth(j))->t_string.c_str();
            
            // Extract and compare layer numbers
            int num_i = atoi(key_i + 5); // +5 to skip "layer"
            int num_j = atoi(key_j + 5);
            
            if (num_i > num_j) {
                // Swap
                node_idx_t temp = layer_keys->nth(i);
                vector_ptr_t new_keys = layer_keys->assoc(i, layer_keys->nth(j));
                new_keys = new_keys->assoc(j, temp);
                layer_keys = new_keys;
            }
        }
    }
    
    // Forward pass through each layer
    for (size_t i = 0; i < layer_keys->size(); i++) {
        node_idx_t layer_key = layer_keys->nth(i);
        node_idx_t layer_idx = model->get(layer_key, node_eq);
        hash_map_ptr_t layer = get_node(layer_idx)->as_hash_map();
        
        // Get layer type
        node_idx_t type_idx = layer->get(new_node_keyword("type"), node_eq);
        if (type_idx == INV_NODE) continue;
        
        const char* layer_type = get_node(type_idx)->t_string.c_str();
        
        // Apply appropriate forward pass based on layer type
        if (strcmp(layer_type, "linear") == 0) {
            current_output = native_nn_linear_forward(env, list_va(layer_idx, current_output));
        }
        else if (strcmp(layer_type, "dropout") == 0) {
            current_output = native_nn_dropout_forward(env, list_va(layer_idx, current_output, TRUE_NODE));
        }
        else if (strcmp(layer_type, "batch-norm") == 0) {
            current_output = native_nn_batch_norm_forward(env, list_va(layer_idx, current_output, TRUE_NODE));
        }
        else if (strcmp(layer_type, "conv1d") == 0) {
            current_output = native_nn_conv1d_forward(env, list_va(layer_idx, current_output));
        }
        else if (strcmp(layer_type, "max-pool1d") == 0) {
            current_output = native_nn_max_pool1d_forward(env, list_va(layer_idx, current_output));
        }
        
        // Apply activation function if specified
        node_idx_t activation_idx = layer->get(new_node_keyword("activation"), node_eq);
        if (activation_idx != INV_NODE) {
            const char* activation_type = get_node(activation_idx)->t_string.c_str();
            
            if (strcmp(activation_type, "relu") == 0) {
                current_output = native_nn_relu(env, list_va(current_output));
            }
            else if (strcmp(activation_type, "sigmoid") == 0) {
                current_output = native_nn_sigmoid(env, list_va(current_output));
            }
            else if (strcmp(activation_type, "tanh") == 0) {
                current_output = native_nn_tanh(env, list_va(current_output));
            }
            else if (strcmp(activation_type, "softmax") == 0) {
                current_output = native_nn_softmax(env, list_va(current_output));
            }
            else if (strcmp(activation_type, "leaky-relu") == 0) {
                double alpha = 0.01; // Default alpha
                node_idx_t alpha_idx = layer->get(new_node_keyword("alpha"), node_eq);
                if (alpha_idx != INV_NODE) {
                    alpha = get_node_float(alpha_idx);
                }
                current_output = native_nn_leaky_relu(env, list_va(current_output, new_node_float(alpha)));
            }
            else if (strcmp(activation_type, "elu") == 0) {
                double alpha = 1.0; // Default alpha
                node_idx_t alpha_idx = layer->get(new_node_keyword("alpha"), node_eq);
                if (alpha_idx != INV_NODE) {
                    alpha = get_node_float(alpha_idx);
                }
                current_output = native_nn_elu(env, list_va(current_output, new_node_float(alpha)));
            }
            else if (strcmp(activation_type, "gelu") == 0) {
                current_output = native_nn_gelu(env, list_va(current_output));
            }
            else if (strcmp(activation_type, "swish") == 0) {
                double beta = 1.0; // Default beta
                node_idx_t beta_idx = layer->get(new_node_keyword("beta"), node_eq);
                if (beta_idx != INV_NODE) {
                    beta = get_node_float(beta_idx);
                }
                current_output = native_nn_swish(env, list_va(current_output, new_node_float(beta)));
            }
        }
    }
    
    return current_output;
}

// Gradient clipping utility
// Arguments: (gradients max_norm)
static node_idx_t native_nn_clip_gradients(env_ptr_t env, list_ptr_t args) {
    list_t::iterator it(args);
    node_idx_t gradients_idx = *it++;
    double max_norm = get_node_float(*it++);
    
    hash_map_ptr_t gradients = get_node(gradients_idx)->as_hash_map();
    hash_map_ptr_t clipped_gradients = new_hash_map();
    
    // Calculate the global norm across all gradient matrices
    double global_norm_squared = 0.0;
    
    // Iterate through layers in the gradients
    for (hash_map_t::iterator layer_it = gradients->begin(); layer_it; layer_it++) {
        node_idx_t layer_key = layer_it->first;
        node_t* layer_node = get_node(layer_it->second);
        
        if (layer_node->is_hash_map()) {
            hash_map_ptr_t layer_grads = layer_node->as_hash_map();
            
            // Iterate through params within the layer
            for (hash_map_t::iterator param_it = layer_grads->begin(); param_it; param_it++) {
                node_t* param_node = get_node(param_it->second);
                
                if (param_node->is_matrix()) {
                    matrix_ptr_t grad_mat = param_node->as_matrix();
                    
                    // Sum up the squared values
                    for (int i = 0; i < grad_mat->width; i++) {
                        for (int j = 0; j < grad_mat->height; j++) {
                            double val = get_node_float(grad_mat->get(i, j));
                            global_norm_squared += val * val;
                        }
                    }
                }
            }
        }
    }
    
    double global_norm = sqrt(global_norm_squared);
    
    // If global norm exceeds max_norm, scale all gradients
    if (global_norm > max_norm) {
        double scale_factor = max_norm / global_norm;
        
        // Iterate through layers again to scale gradients
        for (hash_map_t::iterator layer_it = gradients->begin(); layer_it; layer_it++) {
            node_idx_t layer_key = layer_it->first;
            node_t* layer_node = get_node(layer_it->second);
            
            if (layer_node->is_hash_map()) {
                hash_map_ptr_t layer_grads = layer_node->as_hash_map();
                hash_map_ptr_t clipped_layer_grads = new_hash_map();
                
                // Iterate through params
                for (hash_map_t::iterator param_it = layer_grads->begin(); param_it; param_it++) {
                    node_idx_t param_key = param_it->first;
                    node_t* param_node = get_node(param_it->second);
                    
                    if (param_node->is_matrix()) {
                        matrix_ptr_t grad_mat = param_node->as_matrix();
                        matrix_ptr_t clipped_grad_mat = new_matrix(grad_mat->width, grad_mat->height);
                        
                        // Scale each value
                        for (int i = 0; i < grad_mat->width; i++) {
                            for (int j = 0; j < grad_mat->height; j++) {
                                double val = get_node_float(grad_mat->get(i, j));
                                clipped_grad_mat->set(i, j, new_node_float(val * scale_factor));
                            }
                        }
                        
                        clipped_layer_grads->assoc_inplace(param_key, new_node_matrix(clipped_grad_mat), node_eq);
                    }
                }
                
                clipped_gradients->assoc_inplace(layer_key, new_node_hash_map(clipped_layer_grads), node_eq);
            }
        }
        
        return new_node_hash_map(clipped_gradients);
    }
    
    // If no clipping needed, return original gradients
    return gradients_idx;
}

// Module initialization function
void jo_clojure_nn_init(env_ptr_t env) {
    // Neural network layers
    env->set("nn/linear", new_node_native_function("nn/linear", &native_nn_linear, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/linear-forward", new_node_native_function("nn/linear-forward", &native_nn_linear_forward, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/dropout", new_node_native_function("nn/dropout", &native_nn_dropout, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/dropout-forward", new_node_native_function("nn/dropout-forward", &native_nn_dropout_forward, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/batch-norm", new_node_native_function("nn/batch-norm", &native_nn_batch_norm, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/batch-norm-forward", new_node_native_function("nn/batch-norm-forward", &native_nn_batch_norm_forward, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/conv1d", new_node_native_function("nn/conv1d", &native_nn_conv1d, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/conv1d-forward", new_node_native_function("nn/conv1d-forward", &native_nn_conv1d_forward, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/max-pool1d", new_node_native_function("nn/max-pool1d", &native_nn_max_pool1d, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/max-pool1d-forward", new_node_native_function("nn/max-pool1d-forward", &native_nn_max_pool1d_forward, false, NODE_FLAG_PRERESOLVE));
    
    // Matrix initialization functions
    env->set("nn/init-random-matrix", new_node_native_function("nn/init-random-matrix", &native_nn_init_random_matrix, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/init-bias-matrix", new_node_native_function("nn/init-bias-matrix", &native_nn_init_bias_matrix, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/init-constant-matrix", new_node_native_function("nn/init-constant-matrix", &native_nn_init_constant_matrix, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/init-xavier-matrix", new_node_native_function("nn/init-xavier-matrix", &native_nn_init_xavier_matrix, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/init-he-matrix", new_node_native_function("nn/init-he-matrix", &native_nn_init_he_matrix, false, NODE_FLAG_PRERESOLVE));
    
    // Activation functions
    env->set("nn/relu", new_node_native_function("nn/relu", &native_nn_relu, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/sigmoid", new_node_native_function("nn/sigmoid", &native_nn_sigmoid, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/tanh", new_node_native_function("nn/tanh", &native_nn_tanh, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/softmax", new_node_native_function("nn/softmax", &native_nn_softmax, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/leaky-relu", new_node_native_function("nn/leaky-relu", &native_nn_leaky_relu, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/gelu", new_node_native_function("nn/gelu", &native_nn_gelu, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/elu", new_node_native_function("nn/elu", &native_nn_elu, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/swish", new_node_native_function("nn/swish", &native_nn_swish, false, NODE_FLAG_PRERESOLVE));
    
    // Loss functions
    env->set("nn/mse-loss", new_node_native_function("nn/mse-loss", &native_nn_mse_loss, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/cross-entropy-loss", new_node_native_function("nn/cross-entropy-loss", &native_nn_cross_entropy_loss, false, NODE_FLAG_PRERESOLVE));
    
    // Optimizers
    env->set("nn/sgd", new_node_native_function("nn/sgd", &native_nn_sgd, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/sgd-step", new_node_native_function("nn/sgd-step", &native_nn_sgd_step, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/adam", new_node_native_function("nn/adam", &native_nn_adam, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/adam-step", new_node_native_function("nn/adam-step", &native_nn_adam_step, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/rmsprop", new_node_native_function("nn/rmsprop", &native_nn_rmsprop, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/rmsprop-step", new_node_native_function("nn/rmsprop-step", &native_nn_rmsprop_step, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/adamw", new_node_native_function("nn/adamw", &native_nn_adamw, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/adamw-step", new_node_native_function("nn/adamw-step", &native_nn_adamw_step, false, NODE_FLAG_PRERESOLVE));
    
    // Learning rate schedulers
    env->set("nn/step-lr-scheduler", new_node_native_function("nn/step-lr-scheduler", &native_nn_step_lr_scheduler, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/scheduler-step", new_node_native_function("nn/scheduler-step", &native_nn_scheduler_step, false, NODE_FLAG_PRERESOLVE));
    
    // Training utilities
    env->set("nn/backward", new_node_native_function("nn/backward", &native_nn_backward, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/clip-gradients", new_node_native_function("nn/clip-gradients", &native_nn_clip_gradients, false, NODE_FLAG_PRERESOLVE));
    
    // Model utilities
    env->set("nn/sequential", new_node_native_function("nn/sequential", &native_nn_sequential, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/sequential-forward", new_node_native_function("nn/sequential-forward", &native_nn_sequential_forward, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/save-model", new_node_native_function("nn/save-model", &native_nn_save_model, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/load-model", new_node_native_function("nn/load-model", &native_nn_load_model, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/accuracy", new_node_native_function("nn/accuracy", &native_nn_accuracy, false, NODE_FLAG_PRERESOLVE));
}