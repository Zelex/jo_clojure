; 1. Single Threaded
; 2. Atoms only
; 3. Mutexes
; 4. STM - normal case
; 5. STM - side effects
; 6. STM - Atoms 
; 7. STM - Live Lock
; 8. STM - Dead Lock
; 9. STM - Failed commit crash
; 10. STM - weak Atomicity
; 11. STM - Performance
; 12. STM - Power

; come up with some kind of case where you want to do something in parallel. Where each thing takes roughly the same amount of time.
; These things compete for some central source of atoms or something or whatever.


; the question becomes... what exactly am I going to process what requires synchronization via STM or another mechanism?

; Also, I need to re-implement promises and futures to use atoms so that they are STM compatible.

; Compiling code is probably a good test. 
; Can simulate with random sleeps to some extent.

; ok so compiling code is like
; * list of targets (lib or dll or exe)
; * each target has a list of files
; * you have a list of files to compile for each target
; * linking has dependencies between other link targets, but compiling can all happen in parallel.
; * You have a list of files to link for each target
; * As you go collect errors/warnings/logs
; * should be like a DAG structure.

(def targets [ SuperCoolProgram ])

(def global-lock (atom nil))

(def compile-files-done (atom ()))

(def errors ())
(def warnings ())

(def compiler 'clang)
(def linker 'clang)

(defn compile-file-internal [[file T]]
    ;(println 'Compiling file T)
    ;(System/exec compiler "-c" file "-o" (System/tmpnam))
    (Thread/sleep T)
    ;(print ".")
    true)

(defn compile-result-success [result] true)
    
(defn compile-result-message [result] "")

(defn compile-file-st [filename] 
    (let [compile-result (compile-file-internal filename)]
        (if (compile-result-success compile-result)
            (do (swap! compile-files-done conj filename)
                (compile-result-message compile-result))
            (do (swap! errors cons (compile-result-message compile-result))
                (swap! warnings cons (compile-result-message compile-result))
                (swap! compile-files-done cons filename)))))

(defn compile-file-stm [filename] 
    (dosync
        (let [compile-result (compile-file-internal filename)]
            (if (compile-result-success compile-result)
                (do (swap! compile-files-done conj filename)
                    (compile-result-message compile-result))
                (do (swap! errors conj (compile-result-message compile-result))
                    (swap! warnings conj (compile-result-message compile-result))
                    (swap! compile-files-done conj filename))))))

(defn compile-file-stm-fast [filename] 
    (let [compile-result (compile-file-internal filename)]
        (dosync
            (if (compile-result-success compile-result)
                (do (swap! compile-files-done conj filename)
                    (compile-result-message compile-result))
                (do (swap! errors conj (compile-result-message compile-result))
                    (swap! warnings conj (compile-result-message compile-result))
                    (swap! compile-files-done conj filename))))))

(defn compile-file-atom [filename] 
    (let [compile-result (compile-file-internal filename)]
        (if (compile-result-success compile-result)
            (do (swap! compile-files-done conj filename)
                (compile-result-message compile-result))
            (do (swap! errors conj (compile-result-message compile-result))
                (swap! warnings conj (compile-result-message compile-result))
                (swap! compile-files-done conj filename)))))

(defn compile-file-mutex [filename] 
    (let [compile-result (compile-file-internal filename)]
        (locking global-lock
            (if (compile-result-success compile-result)
                (do (swap! compile-files-done conj filename)
                    (compile-result-message compile-result))
                (do (swap! errors conj (compile-result-message compile-result))
                    (swap! warnings conj (compile-result-message compile-result))
                    (swap! compile-files-done conj filename))))))

(defn compile-file-rand [filename]
    (if (> 0.5 (rand)) 
        (compile-file-stm filename) 
        (compile-file-atom filename)))


;(def files-2 (doall (for [idx (range 1000) :let [T (rand 0.05 0.05)]] [idx T])))
;(def files-3 (doall (for [idx (range 1000) :let [T (rand 1)]] [idx T])))

;(doseq [num-cores (range 1 64)]
    ;(Thread/workers num-cores)
    ;(println (time (doall (map deref (doall (pmap compile-file-stm-fast files-1))))))
;)


;(println "Mutexes")
;(println (time (doall (map deref (doall (pmap compile-file-mutex files))))))

;(println "Atoms Only")
;(println (time (doall (map deref (doall (pmap compile-file-atom files))))))

; STM here almost acts like a partial sort, where the faster things are done first, 
; followed by the slower ones (as slower ones get retried a bunch)
;(println "STM Slow")
;(println (time (doall (map deref (doall (pmap compile-file-stm files))))))

;(println "STM Fast (only parts that need sync, not whole action)")
;(println (time (doall (map deref (doall (pmap compile-file-stm-fast files))))))

;(println "STM Live-Lock situation (slow)")
;(println (time (let [tmp (future (compile-file-stm ["./test/test.c" 1]))]
    ;(doall (map deref (doall (pmap compile-file-stm files))))
    ;(deref tmp))))

;(println "STM/Atom 50/50 random mix")
;(println (time (doall (map deref (doall (pmap compile-file-rand files))))))

(comment

; Test Live-lock situations where things take twice as long as they should
(println "STM Live-Lock simulation (just shows how it fails conceptually)")
(println (time (let 
            [A (atom ()) 
             slow-task (future 
                (dosync 
                    (println "Slow task begin") 
                    (swap! A conj "abc") 
                    (Thread/sleep 2000) 
                    (println "Slow task end")))
            ]
    ; Wait to make sure that the slow task has accessed atom A
    (Thread/sleep 1000)
    ; Now kick off a fast task that will access A as well, causing a commit failure in slow-task
    (deref (future 
        (dosync
            (println "Fast task begin") 
            (swap! A conj "abc") 
            (println "Fast task end"))))
    @slow-task)))

; Test dead-locks
(println "STM Dead-lock simulation")
(let [A (atom false)
    B (atom false)
    thread-1 (future (dosync
        (reset! A true)
        (when @B (println "cannot get here thread-1")) 
        (println "thread-1 done")))
    thread-2 (future (dosync
        (reset! B true)
        (when @A (println "cannot get here thread-2"))
        (println "thread-2 done")))
    ]
    ; Wait for thread-1 and thread-2 to finish by doing a deref on them
    @thread-1 @thread-2)
)

(defn reset-all [] 
    (reset! compile-files-done ())
    (Thread/atom-retries-reset)
    (Thread/stm-retries-reset))

(def files-1 (for [idx (range 1000)] [idx 1]))
;(def files-1 (for [idx (range 1000)] [idx (rand 0.1 2)]))
;(def files-1 (for [idx (range 1000)] [idx (if (< (rand) 0.95) 0.01 20)])) ; Diachomatic

(let [results (doall (for [num-cores (range 1 (+ 1 *hardware-concurrency*))] (do 
        ; Set the number of worker threads...
        (Thread/workers num-cores)
        (println "Testing with" num-cores "cores")
        ; Now lets kick off some STM tests and return results in a vector
        [
            num-cores 
            (time (do (reset-all) (dorun (map deref (doall (pmap compile-file-mutex files-1))))))
            (time (do (reset-all) (dorun (map deref (doall (pmap compile-file-atom files-1))))))
            (Thread/atom-retries)
            (time (do (reset-all) (dorun (map deref (doall (pmap compile-file-stm files-1))))))
            (Thread/stm-retries)
            (time (do (reset-all) (dorun (map deref (doall (pmap compile-file-stm-fast files-1))))))
            (Thread/stm-retries)
        ]
    )))]
    ; Output Results to csv file
    (->> results
        (map (fn [[a b c d e f g h]] (str a ", " b ", " c ", " d ", " e ", " f ", " g ", " h "\n")))
        (reduce str)
        (spit "stm.csv"))

(println "Done")

