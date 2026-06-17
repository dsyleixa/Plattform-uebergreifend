# train_tensorFlow_net.py 
# ältere, jetzt optimierte Version
# (2026-06-14)

# changelog:
# 1. import traceback hinzugefügt.
# 2. Im except-Block wird jetzt traceback.print_exc() ausgegeben.

import sys

# ==============================================================================
# ABSTURZ-SCHUTZ: Fängt alle Fehler ab, damit das Konsolenfenster offen bleibt
# ==============================================================================
try:
    import os
    import pandas as pd
    import numpy as np
    import tensorflow as tf
    from tensorflow.keras import layers, models
    from sklearn.model_selection import train_test_split

    # 1. PFADE DEFINIEREN
    dataset_dir = os.path.dirname(os.path.abspath(__file__))

    csv_path = os.path.join(dataset_dir, "dataset", "tensorflow_dataset.csv")
    tflite_output_path = os.path.join(dataset_dir, "dataset", "model.tflite")
    labels_output_path = os.path.join(dataset_dir, "dataset", "labels.txt")
    cfg_path = os.path.join(dataset_dir, "labels.cfg")

    print(f"Aktuelles Arbeitsverzeichnis: {dataset_dir}", flush=True)
    
    # DYNAMISCHES EINLESEN DER LABELS.CFG
    print("Lade Klassen-Definitionen aus 'labels.cfg'...", flush=True)
    if not os.path.exists(cfg_path):
        print(f"\n[FEHLER] Die Master-Datei '{cfg_path}' wurde nicht gefunden!", flush=True)
        raise FileNotFoundError("labels.cfg fehlt.")

    klassen_spickzettel = {}
    with open(cfg_path, 'r', encoding='utf-8') as f:
        for zeile in f:
            zeile = zeile.strip()
            if not zeile or zeile.startswith('#'):
                continue
            if ':' in zeile:
                teile = zeile.split(':', 1)
                try:
                    class_id = int(teile[0].strip())
                    class_name = teile[1].strip()
                    klassen_spickzettel[class_id] = class_name
                except ValueError:
                    continue

    print(f" -> {len(klassen_spickzettel)} Klassen erfolgreich aus der Konfiguration geladen.", flush=True)

    # 2. CSV-DATEI EINLESEN
    print("Lade Datensatz aus CSV...", flush=True)
    if not os.path.exists(csv_path):
        print(f"\n[FEHLER] Die Datei '{csv_path}' wurde nicht gefunden!", flush=True)
        raise FileNotFoundError("CSV-Datei fehlt.")

    df = pd.read_csv(csv_path)

    pixel_cols = [c for c in df.columns if c.startswith('pixel_')]
    target_cols = [c for c in df.columns if c.startswith('target_')]

    NUM_CLASSES = len(target_cols)

    X = df[pixel_cols].values.astype(np.float32)
    X = X.reshape(-1, 224, 224, 1)
    Y = df[target_cols].values.astype(np.float32)

    print(f"Datensatz erfolgreich geladen. Bilder: {X.shape[0]}, KI-Ausgänge (Slots): {NUM_CLASSES}", flush=True)

    if X.shape[0] < 4:
        print("\n[WARNUNG] Zu wenig Bilder im Datensatz für ein stabiles Training!", flush=True)
        X_train, X_val, Y_train, Y_val = X, X, Y, Y
    else:
        X_train, X_val, Y_train, Y_val = train_test_split(X, Y, test_size=0.2, random_state=42)

    # 3. DAS CNN-MODELL DEFINIEREN
    model = models.Sequential([
        layers.Input(shape=(224, 224, 1)),
        layers.Conv2D(16, (3, 3), activation='relu'),
        layers.MaxPooling2D((2, 2)),
        layers.Conv2D(32, (3, 3), activation='relu'),
        layers.MaxPooling2D((2, 2)),
        layers.Conv2D(64, (3, 3), activation='relu'),
        layers.MaxPooling2D((2, 2)),
        layers.Flatten(),
        layers.Dense(64, activation='relu'),
        layers.Dropout(0.3),
        layers.Dense(NUM_CLASSES, activation='sigmoid')
    ])

    model.compile(optimizer='adam',
                  loss='binary_crossentropy',
                  metrics=['binary_accuracy'])

    # 4. DAS TRAINING STARTEN
    print("\nStarte CNN-Training auf der CPU...", flush=True)
    EPOCHS = 15
    BATCH_SIZE = 4

    # verbose=1 sorgt dafür, dass der Fortschritt zeilenweise ausgegeben wird
    model.fit(X_train, Y_train, 
              epochs=EPOCHS, 
              batch_size=BATCH_SIZE, 
              validation_data=(X_val, Y_val),
              verbose=1)

    # 5. KONVERTIERUNG IN TENSORFLOW LITE (.tflite)
    print("\nKonvertiere Modell zu TensorFlow Lite...", flush=True)
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()

    with open(tflite_output_path, 'wb') as f:
        f.write(tflite_model)

    # 6. DYNAMISCHE LABELS.TXT ERZEUGEN
    print("Erzeuge labels.txt...", flush=True)
    with open(labels_output_path, 'w', encoding='utf-8') as f:
        for i in range(NUM_CLASSES):
            name = klassen_spickzettel.get(i, f"Slot_{i}")
            f.write(f"{name}\n")

    print("\n========================================================", flush=True)
    print("SUCCESS: Training und Export abgeschlossen!", flush=True)
    print(f" -> Modell: {tflite_output_path}", flush=True)
    print(f" -> Labels: {labels_output_path}", flush=True)
    print("========================================================", flush=True)

except Exception as e:
    print("\n========================================================", flush=True)
    print("[KRITISCHER ABSTURZ] Ein Fehler ist aufgetreten:", flush=True)
    print(f"-> {e}", flush=True)
    print("========================================================", flush=True)

finally:
    # NEU: Überprüft, ob das Skript unsichtbar im Hintergrund von C++ gerufen wurde
    if sys.platform == "win32" and not sys.stdin.isatty():
        print("\nTraining über C++ beendet. Schließe Prozess automatisch...", flush=True)
    else:
        # Nur beim manuellen Doppelklick in Windows bleibt das Fenster für dich offen!
        input("\nDrücke ENTER zum Schließen des Verlaufs...")
