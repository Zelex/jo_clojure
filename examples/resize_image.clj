(def from-file (nth *command-line-args* 2))
(def to-file (nth *command-line-args* 3))
(def to-width (nth *command-line-args* 4))
(def to-height (nth *command-line-args* 5))

(println (str "Resizing '" from-file "' to '" to-file "' to " to-width "x" to-height))

(def C (canvas/load-file from-file))
(println "Loaded" (canvas/width C) "x" (canvas/height C) "x" (canvas/channels C) "image")
(def C2 (canvas/resize C to-width to-height))
(println "Resized to" (canvas/width C2) "x" (canvas/height C2) "x" (canvas/channels C2) "image")
(canvas/save-file C2 to-file "jpg")

