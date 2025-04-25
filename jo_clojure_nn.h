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
    
    // Create updated model
    hash_map_ptr_t model = new_hash_map(*get_node(model_idx)->as_hash_map());
    
    // Update each parameter in the model
    for (hash_map_t::iterator it = model->begin(); it; it++) {
        if (gradients->contains(it->first, node_eq)) {
            node_idx_t param = it->second;
            node_idx_t grad = gradients->get(it->first, node_eq);
            node_t *param_node = get_node(param);
            node_t *grad_node = get_node(grad);
            
            if (param_node->is_matrix() && grad_node->is_matrix()) {
                matrix_ptr_t param_mat = param_node->as_matrix();
                matrix_ptr_t grad_mat = grad_node->as_matrix();
                matrix_ptr_t updated_param = new_matrix(param_mat->width, param_mat->height);
                
                for (int i = 0; i < param_mat->width; i++) {
                    for (int j = 0; j < param_mat->height; j++) {
                        double p = get_node_float(param_mat->get(i, j));
                        double g = get_node_float(grad_mat->get(i, j));
                        updated_param->set(i, j, new_node_float(p - lr * g));
                    }
                }
                
                model->assoc_inplace(it->first, new_node_matrix(updated_param), node_eq);
            }
        }
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

// Module initialization function
void jo_clojure_nn_init(env_ptr_t env) {
    // Neural network layers
    env->set("nn/linear", new_node_native_function("nn/linear", &native_nn_linear, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/linear-forward", new_node_native_function("nn/linear-forward", &native_nn_linear_forward, false, NODE_FLAG_PRERESOLVE));
    
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
    
    // Loss functions
    env->set("nn/mse-loss", new_node_native_function("nn/mse-loss", &native_nn_mse_loss, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/cross-entropy-loss", new_node_native_function("nn/cross-entropy-loss", &native_nn_cross_entropy_loss, false, NODE_FLAG_PRERESOLVE));
    
    // Optimizers
    env->set("nn/sgd", new_node_native_function("nn/sgd", &native_nn_sgd, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/sgd-step", new_node_native_function("nn/sgd-step", &native_nn_sgd_step, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/adam", new_node_native_function("nn/adam", &native_nn_adam, false, NODE_FLAG_PRERESOLVE));
    env->set("nn/adam-step", new_node_native_function("nn/adam-step", &native_nn_adam_step, false, NODE_FLAG_PRERESOLVE));
    
    // Training utilities
    env->set("nn/backward", new_node_native_function("nn/backward", &native_nn_backward, false, NODE_FLAG_PRERESOLVE));
} 