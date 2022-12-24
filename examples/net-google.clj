(def conn (net/connect (net/socket) "google.com" 80))
(net/send conn "GET /\n\n")

; skip headers
(loop [s (net/recv-line conn)] 
    (if-not (empty? (trim-newline s)) (recur (net/recv-line conn))))

; print page
(loop [s (net/recv conn)]
  (if-not (empty? s)
    (do (print s) (recur (net/recv conn)))))

(net/close conn)
