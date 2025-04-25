;; XOR Neural Network Example using jo_clojure_nn
;; This example demonstrates training a neural network to solve the XOR problem

;; Create the XOR dataset
(defn generate-xor-dataset []
  (let [;; 4 input samples with 2 features each
        ;; X: width=features(2), height=samples(4)
        X-initial (matrix 2 4)
        ;; y: width=output_features(1), height=samples(4)
        y-initial (matrix 1 4)
        
        ;; Capture the updated matrices after setting columns
        X (-> X-initial
              (matrix/set-col 0 [0.0 0.0 1.0 1.0])
              (matrix/set-col 1 [0.0 1.0 0.0 1.0]))
        
        ;; Capture the updated y matrix after setting column
        y (matrix/set-col y-initial 0 [0.0 1.0 1.0 0.0])]
    
    (println "X:" X)
    (println "y:" y)
    
    {:X X :y y}))

;; Create a neural network model for XOR
;; We need at least one hidden layer since XOR is not linearly separable
(defn create-model []
  (let [;; Create layer1 with larger hidden size (2 -> 8) - increased from 4
        weights1 (nn/init-he-matrix 8 2)  ;; Using He initialization (rows=8, cols=2, fan_in=2)
        bias1 (nn/init-bias-matrix 8 1 0.01)
        layer1 {:type :linear
                :activation :relu  ;; Explicitly specify activation
                :weights weights1
                :bias bias1}
        
        ;; Create output layer (8 -> 1) - updated from 4 -> 1
        weights2 (nn/init-he-matrix 1 8 8)  ;; Using He initialization (rows=1, cols=8, fan_in=8)
        bias2 (nn/init-constant-matrix 1 1 0.01)
        layer2 {:type :linear
                :activation :sigmoid  ;; Explicitly specify activation
                :weights weights2
                :bias bias2}]
    
    {:layer1 layer1
     :layer2 layer2}))

;; Forward pass through the model
(defn forward [model X]
  (let [;; X is already (width=2, height=4)
        layer1 (get model :layer1)
        h1 (nn/linear-forward layer1 X) ; First hidden layer output
        h1-act (nn/relu h1)
        
        layer2 (get model :layer2)  
        output (nn/linear-forward layer2 h1-act) ; Output layer
        output-act (nn/sigmoid output)]
    output-act))

;; Calculate loss
(defn loss-fn [model X y]
  (let [pred (forward model X) ; pred: (width=1, height=4)
        ] ; y: (width=1, height=4)
    ;; Use Cross-Entropy Loss
    (nn/cross-entropy-loss pred y)))

;; Train the model
(defn train-model [model X y epochs learning-rate]
  (println "Training XOR model...")
  
  ;; Use Adam optimizer with better parameters
  (let [optimizer (nn/adam model learning-rate 0.9 0.999 1e-8)]
    
    (loop [epoch 1
           current-optimizer optimizer
           best-loss Float/MAX_VALUE
           best-model model
           patience 0 
           max-patience 100]  ;; Early stopping if no improvement
      
      (if (or (> epoch epochs) (> patience max-patience))
        ;; Return the best model found during training
        best-model
        
        (let [current-model (get current-optimizer :model)
              ;; Calculate loss
              current-loss (loss-fn current-model X y)
              
              ;; Calculate gradients
              gradients (nn/backward current-model X y)
              
              ;; Update parameters
              updated-optimizer (nn/adam-step current-optimizer gradients)
              
              ;; Track best model based on loss
              [new-best-loss new-best-model new-patience] 
              (if (< current-loss best-loss)
                [current-loss current-model 0]  ;; Improved, reset patience
                [best-loss best-model (inc patience)])]  ;; No improvement
          
          ;; Print progress every 100 epochs
          (when (= (mod epoch 100) 0)
            (println "Epoch" epoch "- Loss:" current-loss 
                     (if (= new-patience 0) " (improved)" "")))
          
          ;; Continue training
          (recur (inc epoch) updated-optimizer new-best-loss new-best-model new-patience))))))

;; Evaluate and print predictions
(defn evaluate-model [model X y]
  (let [pred (forward model X) ; pred: (width=1, height=4)
        pred-col (matrix/get-col pred 0) ; Get the first (and only) column as a vector
        y-col (matrix/get-col y 0)       ; Get target column as a vector
        ;; Use Cross-Entropy Loss for evaluation as well
        loss (nn/cross-entropy-loss pred y)] ; y: (width=1, height=4)
    
    (println "\nEvaluation results:")
    (println "Loss:" loss)
    
    (println "\nPredictions:")
    (println "Input [0,0] -> Expected: 0, Predicted:" (nth pred-col 0))
    (println "Input [0,1] -> Expected: 1, Predicted:" (nth pred-col 1))
    (println "Input [1,0] -> Expected: 1, Predicted:" (nth pred-col 2))
    (println "Input [1,1] -> Expected: 0, Predicted:" (nth pred-col 3))
    
    ;; Convert sigmoid outputs to binary predictions (threshold at 0.5)
    (let [binary-preds (matrix 1 4)] ; width=1, height=4
      (let [binary-pred-vals [(if (> (nth pred-col 0) 0.5) 1.0 0.0)
                              (if (> (nth pred-col 1) 0.5) 1.0 0.0)
                              (if (> (nth pred-col 2) 0.5) 1.0 0.0)
                              (if (> (nth pred-col 3) 0.5) 1.0 0.0)]]
        (let [populated-binary-preds (matrix/set-col binary-preds 0 binary-pred-vals)]
          (println "\nBinary predictions:")
          ;; Get the column vector from the *populated* binary-preds matrix
          (let [binary-preds-col (matrix/get-col populated-binary-preds 0)]
            (println "Input [0,0] -> Expected: 0, Predicted:" (nth binary-preds-col 0))
            (println "Input [0,1] -> Expected: 1, Predicted:" (nth binary-preds-col 1))
            (println "Input [1,0] -> Expected: 1, Predicted:" (nth binary-preds-col 2))
            (println "Input [1,1] -> Expected: 0, Predicted:" (nth binary-preds-col 3))
            
            ;; Calculate accuracy
            (let [correct (matrix 1 4)] ; width=1, height=4
              ;; Use the already extracted binary-preds-col
              (let [correct-vals [(if (= (nth binary-preds-col 0) (nth y-col 0)) 1.0 0.0)
                                    (if (= (nth binary-preds-col 1) (nth y-col 1)) 1.0 0.0)
                                    (if (= (nth binary-preds-col 2) (nth y-col 2)) 1.0 0.0)
                                    (if (= (nth binary-preds-col 3) (nth y-col 3)) 1.0 0.0)]]
                (let [populated-correct (matrix/set-col correct 0 correct-vals)]
                  ;; Get the correct column vector to calculate the sum
                  (let [correct-col (matrix/get-col populated-correct 0)
                        accuracy (/ (+ (nth correct-col 0)
                                       (nth correct-col 1)
                                       (nth correct-col 2)
                                       (nth correct-col 3))
                                     4.0)]
                    (println "\nAccuracy:" accuracy)))))))))))

;; Run the XOR example
(defn run-example []
  (println "XOR Neural Network Example")
  (println "-------------------------")
  
  ;; Generate XOR dataset
  (println "Generating XOR dataset...")
  (let [dataset (generate-xor-dataset)
        X (get dataset :X)
        y (get dataset :y)]
    
    ;; Create model
    (println "Creating model...")
    (let [model (create-model)]
      
      ;; Initial evaluation
      (println "\nBefore training:")
      (evaluate-model model X y)
      
      ;; Train model with increased epochs and adjusted learning rate
      (let [trained-model (train-model model X y 1000 0.1)]
        
        ;; Final evaluation
        (println "\nAfter training:")
        (evaluate-model trained-model X y)))))

;; Execute the example
(run-example) 