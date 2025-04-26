;; Regression Neural Network Example using jo_clojure_nn
;; This example demonstrates training a neural network to approximate a sine function

;; Generate a dataset of sin(x) values
(defn generate-sine-dataset [samples]
  (let [X-initial (matrix 1 samples)
        y-initial (matrix 1 samples)
        
        ;; Generate evenly spaced x values from 0 to 2π
        x-values (mapv #(* % (/ (* 2 Math/PI) (dec samples))) (range samples))
        
        ;; Calculate sin(x) for each x
        y-values (mapv #(Math/sin %) x-values)
        
        ;; Set the matrices
        X (matrix/set-col X-initial 0 x-values)
        y (matrix/set-col y-initial 0 y-values)]
    
    (println "X (first 5):" (take 5 (matrix/get-col X 0)))
    (println "y (first 5):" (take 5 (matrix/get-col y 0)))
    
    {:X X :y y}))

;; Define neural network layer types - PyTorch-like interface
(defn Linear [in-features out-features]
  {:type :linear
   :in-features in-features
   :out-features out-features
   :weights (nn/init-he-matrix out-features in-features in-features)
   :bias (nn/init-bias-matrix out-features 1 0.01)
   :forward (fn [layer input]
              (nn/linear-forward layer input))})

;; Activations
(def activations
  {:relu {:fn nn/relu :backward nn/relu-backward}
   :tanh {:fn nn/tanh :backward nn/tanh-backward}
   :none {:fn identity :backward (fn [_ dy] dy)}})

;; Create a neural network model for regression - PyTorch style
(defn create-model []
  (nn/sequential
    (assoc (Linear 1 16) :activation :tanh)
    (assoc (Linear 16 32) :activation :tanh)
    (assoc (Linear 32 16) :activation :tanh)
    (assoc (Linear 16 1) :activation :none)))

;; Define optimizer - PyTorch style
(defn Adam [model learning-rate & {:keys [beta1 beta2 epsilon]
                                  :or {beta1 0.9 beta2 0.999 epsilon 1e-8}}]
  (nn/adam model learning-rate beta1 beta2 epsilon))

;; Train function - More PyTorch-like
(defn train [model dataset optimizer epochs & {:keys [verbose] :or {verbose true}}]
  (let [X (get dataset :X)
        y (get dataset :y)]
    
    (println "Training regression model...")
    
    (loop [epoch 1
           current-optimizer optimizer
           best-loss Float/MAX_VALUE
           best-model model
           patience 0 
           max-patience 100]
      (if (or (> epoch epochs) (> patience max-patience))
        best-model
        
        (let [current-model (get current-optimizer :model)
              pred (nn/sequential-forward current-model X)
              current-loss (nn/mse-loss pred y)
              gradients (nn/backward current-model X y)
              updated-optimizer (nn/adam-step current-optimizer gradients)
              
              [new-best-loss new-best-model new-patience] 
              (if (< current-loss best-loss)
                [current-loss current-model 0]
                [best-loss best-model (inc patience)])]
          
          (when (and verbose (= (mod epoch 100) 0))
            (println "Epoch" epoch "- Loss:" current-loss 
                     (if (= new-patience 0) " (improved)" "")))
          
          (recur (inc epoch) updated-optimizer new-best-loss new-best-model new-patience))))))

;; Evaluate and visualize predictions
(defn evaluate-model [model dataset]
  (let [X (get dataset :X)
        y (get dataset :y)
        pred (nn/sequential-forward model X)
        loss (nn/mse-loss pred y)]
    
    (println "\nEvaluation results:")
    
    ;; Print a few examples of predictions
    (println "\nSample predictions (x, true y, predicted y):")
    (let [x-vals (matrix/get-col X 0)
          y-vals (matrix/get-col y 0)
          pred-vals (matrix/get-col pred 0)]
      (doseq [i (range 0 (count x-vals) (/ (count x-vals) 10))]
        (let [idx (int i)]
          (println (format "x: %.3f, true: %.3f, pred: %.3f" 
                           (nth x-vals idx) 
                           (nth y-vals idx) 
                           (nth pred-vals idx))))))))

;; Run the regression example
(defn run-example []
  (println "Sine Regression Neural Network Example")
  (println "--------------------------------------")
  
  ;; Generate sine dataset with 100 samples
  (println "Generating sine dataset...")
  (let [dataset (generate-sine-dataset 100)]
    
    ;; Create model - PyTorch style
    (println "Creating model...")
    (let [model (create-model)
          
          ;; Create optimizer - PyTorch style
          optimizer (Adam model 0.01)]
      
      ;; Initial evaluation
      (println "\nBefore training:")
      (evaluate-model model dataset)
      
      ;; Train model - PyTorch style
      (let [trained-model (train model dataset optimizer 200)]
        
        ;; Final evaluation
        (println "\nAfter training:")
        (evaluate-model trained-model dataset)))))

;; Execute the example
(run-example) 