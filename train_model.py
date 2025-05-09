import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
import os

# === Paramètres ===
img_height, img_width = 64, 64
batch_size = 32
data_dir = "dataset"  # chemin vers ton dossier d’images

# === Chargement du dataset ===
train_ds = tf.keras.preprocessing.image_dataset_from_directory(
    data_dir,
    validation_split=0.2,
    subset="training",
    seed=123,
    image_size=(img_height, img_width),
    batch_size=batch_size
)

val_ds = tf.keras.preprocessing.image_dataset_from_directory(
    data_dir,
    validation_split=0.2,
    subset="validation",
    seed=123,
    image_size=(img_height, img_width),
    batch_size=batch_size
)

# === Prétraitement ===
AUTOTUNE = tf.data.AUTOTUNE
train_ds = train_ds.cache().shuffle(1000).prefetch(buffer_size=AUTOTUNE)
val_ds = val_ds.cache().prefetch(buffer_size=AUTOTUNE)

# === Modèle CNN simple ===
model = keras.Sequential([
    layers.Rescaling(1./255, input_shape=(img_height, img_width, 3)),
    layers.Conv2D(32, 3, activation='relu'),
    layers.MaxPooling2D(),
    layers.Conv2D(64, 3, activation='relu'),
    layers.MaxPooling2D(),
    layers.Flatten(),
    layers.Dense(128, activation='relu'),
    layers.Dense(10)  # 10 classes : chiffres de 0 à 9
])

# === Compilation et entraînement ===
model.compile(
    optimizer='adam',
    loss=tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True),
    metrics=['accuracy']
)

model.fit(train_ds, validation_data=val_ds, epochs=10)

# === Sauvegarde du modèle .h5 ===
model.save("model.h5")

# === Conversion en float16 .tflite ===
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.target_spec.supported_types = [tf.float16]
tflite_model = converter.convert()

with open("model_float16.tflite", "wb") as f:
    f.write(tflite_model)

print("✅ Modèle converti en model_float16.tflite")
