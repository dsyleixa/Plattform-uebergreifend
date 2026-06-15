# run_inference.py

import os
import sys
import base64
import numpy as np
import tensorflow as tf

def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    model_path = os.path.join(base_dir, "dataset", "model.tflite")
    labels_path = os.path.join(base_dir, "dataset", "labels.txt")

    if not os.path.exists(model_path) or not os.path.exists(labels_path):
        # flush=True zwingt Python 3.13, den Text SOFORT an C++ abzufangen
        print("RESULT: Fehler - model.tflite oder labels.txt fehlt!", flush=True)
        return

    # Labels laden
    with open(labels_path, 'r', encoding='utf-8') as f:
        labels = [line.strip() for line in f if line.strip()]

    # TFLite Interpreter laden und Speicher zuweisen
    interpreter = tf.lite.Interpreter(model_path=model_path)
    interpreter.allocate_tensors()

    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    # Endlosschleife für eintreffende Base64-Bilder
    while True:
        line = sys.stdin.readline()
        if not line:
            break  # C++ hat den Prozess geschlossen oder gestoppt
        
        line = line.strip()
        if line.startswith("DATA:"):
            try:
                # Text dekodieren
                b64_data = line[5:]
                raw_bytes = base64.b64decode(b64_data)
                
                # In NumPy-Array gießen
                input_data = np.frombuffer(raw_bytes, dtype=np.uint8).astype(np.float32)
                input_data = input_data.reshape(1, 224, 224, 1) / 255.0
                
                # Inferenz berechnen
                interpreter.set_tensor(input_details[0]['index'], input_data)
                interpreter.invoke()
                
                # Ergebnisse abgreifen (Batch auflösen)
                output_data = interpreter.get_tensor(output_details[0]['index'])[0]
                
                # Muster filtern (> 50% Wahrscheinlichkeit)
                ergebnisse = []
                for idx, prob in enumerate(output_data):
                    if prob > 0.5 and idx < len(labels):
                        ergebnisse.append(f"{labels[idx]} ({prob*100:.1f}%)")
                
                # Rückmeldung an das Qt C++ Hauptprogramm mit absolutem Sofort-Flush
                if not ergebnisse:
                    print("RESULT: Nichts erkannt", flush=True)
                else:
                    print(f"RESULT: {', '.join(ergebnisse)}", flush=True)
                
            except Exception as e:
                print(f"RESULT: Fehler beim Dekodieren ({e})", flush=True)

if __name__ == '__main__':
    main()
