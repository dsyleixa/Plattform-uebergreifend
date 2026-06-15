import sys
import base64
import numpy as np
import tensorflow as tf
import os

def main():
    base_dir = os.getcwd()
    model_path = os.path.join(base_dir, 'dataset', 'model.tflite')
    labels_path = os.path.join(base_dir, 'dataset', 'labels.txt')

    if not os.path.exists(model_path) or not os.path.exists(labels_path):
        print('RESULT: Fehler - model.tflite oder labels.txt fehlt!', flush=True)
        return

    labels = []
    with open(labels_path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line: continue
            if ':' in line: line = line.split(':', 1)[-1].strip()
            elif '=' in line: line = line.split('=', 1)[-1].strip()
            labels.append(str(line))

    interpreter = tf.lite.Interpreter(model_path=model_path)
    interpreter.allocate_tensors()
    
    # ABSOLUTE ABSICHERUNG: Loest den TFLite-Details Typenkonflikt bei TF 2.16+ / 2.18+
    in_details = interpreter.get_input_details()
    out_details = interpreter.get_output_details()
    
    if isinstance(in_details, list):
        input_index = in_details[0]['index']
    else:
        first_key = list(in_details.keys())[0]
        input_index = in_details[first_key]['index']
        
    if isinstance(out_details, list):
        output_index = out_details[0]['index']
    else:
        first_key = list(out_details.keys())[0]
        output_index = out_details[first_key]['index']

    while True:
        raw_line = sys.stdin.buffer.readline()
        if not raw_line: break
        try:
            line = raw_line.decode('utf-8', errors='ignore').strip()
        except Exception:
            continue
            
        if line.startswith('DATA:'):
            try:
                b64_data = line[5:]
                raw_bytes = base64.b64decode(b64_data)
                input_data = np.frombuffer(raw_bytes, dtype=np.uint8).astype(np.float32)
                input_data = input_data.reshape(1, 224, 224, 1) / 255.0
                
                interpreter.set_tensor(input_index, input_data)
                interpreter.invoke()
                output_data = interpreter.get_tensor(output_index)
                
                flat_output = output_data.flatten()
                max_idx = int(np.argmax(flat_output))
                prob = float(flat_output[max_idx])
                
                if prob > 0.5 and 0 <= max_idx < len(labels):
                    label_text = str(labels[max_idx])
                    print('RESULT: ' + label_text + ' (' + str(round(prob*100, 1)) + '%)', flush=True)
                else:
                    print('RESULT: Nichts erkannt', flush=True)
            except Exception as e:
                print('RESULT: Fehler bei Inferenz (' + str(e) + ')', flush=True)

if __name__ == '__main__':
    main()
