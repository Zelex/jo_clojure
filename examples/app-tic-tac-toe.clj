(def field (atom [0 0 0 0 0 0 0 0 0]))

(def sokol-desc {
    :width 640
    :height 480
    :window_title "Basic Tic-Tac-Toe App"
    :init_cb (fn [] 
    )
    :frame_cb (fn [] 
        (sgl/viewport 0 0 640 480 false)
        (sgl/defaults)
        (sgl/begin-lines)
        ; draw the grid
        (sgl/v2f  0.33  -1)
        (sgl/v2f  0.33   1)
        (sgl/v2f  -0.33 -1)
        (sgl/v2f  -0.33  1)
        (sgl/v2f  -1     0.33)
        (sgl/v2f   1     0.33)
        (sgl/v2f  -1    -0.33)
        (sgl/v2f   1    -0.33)
        (sgl/end)
        
        ; draw the Xs and Os
        (doseq [i (range 9)]
            (let [x (mod i 3)
                  y (/ i 3)
                  v (get @field i)]
                (when (= v 1)
                    ; draw an O as a circle
                    (sgl/begin-line-strip)
                    (doseq [i (range 32)]
                        (let [a (* 2.0 3.14159265358979323846 (/ i 32.0))]
                            (sgl/v2f-c3b (+ -1 0.33 (* x 0.66) (* 0.33 (Math/sin a))) (+ -1 0.33 (* y 0.66) (* 0.33 (Math/cos a))) 0 0 255)
                        )
                    )
                    (sgl/end)
                )
                (when (= v 2)
                    ; draw an X as a cross
                    (sgl/begin-lines)
                    (sgl/v2f-c3b  (+ -1 0.33 -0.33 (* x 0.66)) (+ -1 0.33 -0.33 (* y 0.66)) 0 0 255)
                    (sgl/v2f-c3b  (+ -1 0.33  0.33 (* x 0.66)) (+ -1 0.33  0.33 (* y 0.66)) 0 0 255)
                    (sgl/v2f-c3b  (+ -1 0.33 -0.33 (* x 0.66)) (+ -1 0.33  0.33 (* y 0.66)) 0 0 255)
                    (sgl/v2f-c3b  (+ -1 0.33  0.33 (* x 0.66)) (+ -1 0.33 -0.33 (* y 0.66)) 0 0 255)
                    (sgl/end)
                )
            )
        )

    )
    :cleanup_cb (fn [] 
    )
    :event_cb (fn [event] 
        (let [
            type (get event :type)
            mouse_x (get event :mouse_x)
            mouse_y (get event :mouse_y)
            mouse_button (get event :mouse_button)
            ]
            (when (= type sokol/SAPP_EVENTTYPE_MOUSE_DOWN)
                (when (= mouse_button sokol/SAPP_MOUSEBUTTON_LEFT)
                    (let [x (int (/ (+ 1 mouse_x) 0.66))
                          y (int (/ (+ 1 mouse_y) 0.66))]
                        (when (and (>= x 0) (< x 3) (>= y 0) (< y 3))
                            (let [i (+ (* y 3) x)]
                                (when (= (get @field i) 0)
                                    (swap! field assoc i 1)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    :fail_cb (fn [msg] 
        (println "Error/Sokol:" msg)
    )
})

(sokol/run sokol-desc)