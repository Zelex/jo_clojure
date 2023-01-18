#!/usr/local/bin/basic 
(def left-file (nth *command-line-args* 2))
(def right-file (nth *command-line-args* 3))

(def exposure (atom 4))
(def zoom (atom 1))

(def left-image (canvas/load-file left-file))
(def right-image (canvas/load-file right-file))
(def diff-image (atom (canvas/diff left-image right-image @exposure)))

(def WIDTH (atom (canvas/width left-image)))
(def HEIGHT (atom (canvas/height left-image)))

(def left-image-sg (atom nil))
(def right-image-sg (atom nil))
(def diff-image-sg (atom nil))

(def mouse-left (atom false))
(def mouse-middle (atom false))
(def mouse-right (atom false))

(def at-x (atom 0))
(def at-y (atom 0))


(def sokol-desc {
    :width @WIDTH
    :height @HEIGHT
    :window_title "Basic App"
    :init_cb (fn [] 
        (reset! left-image-sg (sg/canvas-image left-image))
        (reset! right-image-sg (sg/canvas-image right-image))
        (reset! diff-image-sg (sg/canvas-image @diff-image))
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
                ;:minus (do
                    ;(reset! zoom (Math/clip (- @zoom 0.1) 0.1 1))
                    ;(reset! WIDTH (Math/round (* (canvas/width left-image) @zoom)))
                    ;(reset! HEIGHT (Math/round (* (canvas/height left-image) @zoom)))
                    ;(sokol/set-window-size @WIDTH @HEIGHT)
                ;)
                ;:equal (do
                    ;(reset! zoom (Math/clip (+ @zoom 0.1) 0.1 1))
                    ;(reset! WIDTH (Math/round (* (canvas/width left-image) @zoom)))
                    ;(reset! HEIGHT (Math/round (* (canvas/height left-image) @zoom)))
                    ;(sokol/set-window-size @WIDTH @HEIGHT)
                ;)
                :left-bracket (do 
                    (reset! exposure (Math/clip (- @exposure 1) 1 10))
                    (reset! diff-image (canvas/diff left-image right-image @exposure))
                    (sg/destroy-image @diff-image-sg)
                    (reset! diff-image-sg (sg/canvas-image @diff-image))
                    ;(sg/update-canvas-image @diff-image-sg @diff-image)
                )
                :right-bracket (do 
                    (reset! exposure (Math/clip (+ @exposure 1) 1 10))
                    (reset! diff-image (canvas/diff left-image right-image @exposure))
                    (sg/destroy-image @diff-image-sg)
                    (reset! diff-image-sg (sg/canvas-image @diff-image))
                    ;(sg/update-canvas-image @diff-image-sg @diff-image)
                )
                :escape (sokol/quit)
            )
            :resized (do
                (reset! WIDTH (:window-width event))
                (reset! HEIGHT (:window-height event))
            )
            :mouse-down (do
                (case (:mouse-button event)
                    :left (reset! mouse-left true)
                    :middle (reset! mouse-middle true)
                    :right (reset! mouse-right true)
                )
            )
            :mouse-up (do
                (case (:mouse-button event)
                    :left (reset! mouse-left false)
                    :middle (reset! mouse-middle false)
                    :right (reset! mouse-right false)
                )
            )
            :mouse-move (do
                (when @mouse-left
                    (reset! at-x (Math/clip (/ (- (* @at-x @WIDTH) (:mouse-dx event)) @WIDTH) 0 0.66))
                    (reset! at-y (Math/clip (/ (- (* @at-y @HEIGHT) (:mouse-dy event)) @HEIGHT) 0 0.66))
                )
            )
        )
    )
    :fail_cb (fn [msg] 
        (println "Error/Sokol:" msg)
    )
})

(if (or (nil? left-file) (nil? right-file))
    (println "Usage: cmp-image.clj left-image right-image")
    (sokol/run sokol-desc)
)

