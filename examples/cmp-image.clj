(def left-file (nth *command-line-args* 2))
(def right-file (nth *command-line-args* 3))


(def left-image (canvas/load-file left-file))
(def right-image (canvas/load-file right-file))
(def diff-image (canvas/diff left-image right-image))

(def WIDTH (atom (canvas/width left-image)))
(def HEIGHT (atom (canvas/height left-image)))

(def left-image-sg (atom nil))
(def right-image-sg (atom nil))
(def diff-image-sg (atom nil))

(def at-x (atom 0))
(def at-y (atom 0))

(def sokol-desc {
    :width @WIDTH
    :height @HEIGHT
    :window_title "Basic App"
    :init_cb (fn [] 
        (reset! left-image-sg (sg/canvas-image left-image))
        (reset! right-image-sg (sg/canvas-image right-image))
        (reset! diff-image-sg (sg/canvas-image diff-image))
    )
    :frame_cb (fn [] 
        (sgl/viewport 0 0 @WIDTH @HEIGHT false)
        (sgl/defaults)

        (sgl/enable-texture)

        (sgl/texture @left-image-sg)
        (sgl/begin-quads)
        (sgl/v2f-t2f-c3b -1,1      @at-x,0              255,255,255)
        (sgl/v2f-t2f-c3b -0.33,1   (+ @at-x 0.33),0     255,255,255)
        (sgl/v2f-t2f-c3b -0.33,-1  (+ @at-x 0.33),1     255,255,255)
        (sgl/v2f-t2f-c3b -1,-1     @at-x,1              255,255,255)
        (sgl/end)

        (sgl/texture @diff-image-sg)
        (sgl/begin-quads)
        (sgl/v2f-t2f-c3b -0.33,1   @at-x,0              255,255,255)
        (sgl/v2f-t2f-c3b 0.33,1    (+ @at-x 0.33),0     255,255,255)
        (sgl/v2f-t2f-c3b 0.33,-1   (+ @at-x 0.33),1     255,255,255)
        (sgl/v2f-t2f-c3b -0.33,-1  @at-x,1              255,255,255)
        (sgl/end)

        (sgl/texture @right-image-sg)
        (sgl/begin-quads)
        (sgl/v2f-t2f-c3b 0.33,1   @at-x,0               255,255,255)
        (sgl/v2f-t2f-c3b 1,1      (+ @at-x 0.33),0      255,255,255)
        (sgl/v2f-t2f-c3b 1,-1     (+ @at-x 0.33),1      255,255,255)
        (sgl/v2f-t2f-c3b 0.33,-1  @at-x,1               255,255,255)
        (sgl/end)
    )
    :cleanup_cb (fn [] 
    )
    :event_cb (fn [event] 
        (case (:type event)
            :key-down (case (:key event)
                :escape (sokol/quit)
            )
            :resized (do
                (reset! WIDTH (:window-width event))
                (reset! HEIGHT (:window-height event))
            )
            :mouse-move (do
                (reset! at-x (Math/clip (/ (float (:mouse-x event)) (float @WIDTH)) 0 0.66))
                (reset! at-y (Math/clip (/ (float (:mouse-y event)) (float @HEIGHT)) 0 0.66))
            )
        )
    )
    :fail_cb (fn [msg] 
        (println "Error/Sokol:" msg)
    )
})

(sokol/run sokol-desc)
