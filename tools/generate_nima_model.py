#!/usr/bin/env python3
"""
Generate a darktable .dtmodel package for NIMA (Neural Image Assessment).

This script converts Keras/TensorFlow NIMA models to ONNX and packages them
into darktable's .dtmodel format (a zip archive with config.json + model.onnx).

Usage:
    python3 generate_nima_model.py --backbone mobilenet --weights weights/mobilenet_weights.h5
    python3 generate_nima_model.py --backbone inception_resnet --weights weights/inception_resnet_weights.h5

For Inception ResNet v2, you need to provide trained weights.
For MobileNet, pre-trained weights can be downloaded from:
    https://github.com/idealo/image-quality-assessment/releases
"""

import argparse
import json
import os
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path


def generate_config(model_id, name, backbone, input_size=224):
    """Generate config.json for the model package."""
    config = {
        "id": model_id,
        "name": name,
        "description": f"NIMA aesthetic image quality scorer ({backbone})",
        "task": "rating",
        "backend": "onnx",
        "arch": backbone,
        "version": "1.0",
        "num_inputs": 1,
        "attributes": {
            "input_size": input_size,
            "normalize": True,
            "nima_output": True
        },
        "model_card": {
            "long_description": "Neural Image Assessment (NIMA) model for predicting aesthetic image quality scores. Outputs a 10-class probability distribution for scores 1-10.",
            "scope": "aesthetic image quality assessment",
            "author": "based on Talebi & Milanfar (2017)",
            "source": "https://github.com/idealo/image-quality-assessment",
            "paper": "https://arxiv.org/abs/1709.05424",
            "license": "MIT",
            "training_data": "AVA Dataset",
            "notes": f"Converted from Keras {backbone} weights to ONNX for darktable"
        }
    }
    return config


def convert_mobilenet_to_onnx(weights_path, output_path):
    """Convert MobileNet NIMA model from Keras weights to ONNX."""
    try:
        import tensorflow as tf
        from tensorflow import keras
        from tensorflow.keras import layers
        import tf2onnx
    except ImportError as e:
        print(f"Error: {e}")
        print("Please install TensorFlow and tf2onnx:")
        print("  pip install tensorflow tf2onnx")
        return False

    print("Building MobileNet NIMA model...")

    # Build MobileNet model with NIMA head
    base_model = keras.applications.MobileNet(
        input_shape=(224, 224, 3),
        include_top=False,
        pooling='avg',
        weights=None
    )
    x = layers.Dropout(0.75)(base_model.output)
    x = layers.Dense(10, activation='softmax')(x)
    model = keras.Model(inputs=base_model.input, outputs=x)

    print(f"Loading weights from {weights_path}...")
    model.load_weights(weights_path)

    print("Converting to ONNX...")
    spec = (tf.TensorSpec((1, 224, 224, 3), tf.float32, name="input"),)
    model_proto, _ = tf2onnx.convert.from_keras(model, input_signature=spec, opset=13)

    with open(output_path, "wb") as f:
        f.write(model_proto.SerializeToString())

    print(f"ONNX model saved to {output_path}")
    return True


def convert_inception_resnet_to_onnx(weights_path, output_path):
    """Convert Inception ResNet v2 NIMA model from Keras weights to ONNX."""
    try:
        import tensorflow as tf
        from tensorflow import keras
        from tensorflow.keras import layers
        import tf2onnx
    except ImportError as e:
        print(f"Error: {e}")
        print("Please install TensorFlow and tf2onnx:")
        print("  pip install tensorflow tf2onnx")
        return False

    print("Building Inception ResNet v2 NIMA model...")

    base_model = keras.applications.InceptionResNetV2(
        input_shape=(None, None, 3),
        include_top=False,
        pooling='avg',
        weights=None
    )
    x = layers.Dropout(0.75)(base_model.output)
    x = layers.Dense(10, activation='softmax')(x)
    model = keras.Model(inputs=base_model.input, outputs=x)

    print(f"Loading weights from {weights_path}...")
    model.load_weights(weights_path)

    print("Converting to ONNX...")
    # Note: Inception ResNet v2 uses variable input size
    spec = (tf.TensorSpec((1, 224, 224, 3), tf.float32, name="input"),)
    model_proto, _ = tf2onnx.convert.from_keras(model, input_signature=spec, opset=13)

    with open(output_path, "wb") as f:
        f.write(model_proto.SerializeToString())

    print(f"ONNX model saved to {output_path}")
    return True


def package_dtmodel(model_dir, output_path):
    """Package model directory into a .dtmodel zip file."""
    model_id = os.path.basename(model_dir)
    with zipfile.ZipFile(output_path, 'w', zipfile.ZIP_DEFLATED) as zf:
        for root, dirs, files in os.walk(model_dir):
            for file in files:
                file_path = os.path.join(root, file)
                arcname = os.path.relpath(file_path, os.path.dirname(model_dir))
                zf.write(file_path, arcname)
    print(f"Package saved to {output_path}")


def main():
    parser = argparse.ArgumentParser(description='Generate NIMA .dtmodel for darktable')
    parser.add_argument('--backbone', choices=['mobilenet', 'inception_resnet'],
                        default='mobilenet',
                        help='Model backbone architecture (default: mobilenet)')
    parser.add_argument('--weights', required=True,
                        help='Path to Keras weights file (.h5)')
    parser.add_argument('--output-dir', default='.',
                        help='Output directory for the .dtmodel file')
    parser.add_argument('--model-id',
                        help='Model ID (default: rating-aesthetic-v1)')
    args = parser.parse_args()

    model_id = args.model_id or 'rating-aesthetic-v1'
    model_name = f"aesthetic rating v1 ({args.backbone})"
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmpdir:
        model_dir = os.path.join(tmpdir, model_id)
        os.makedirs(model_dir)

        # Generate config.json
        config = generate_config(model_id, model_name, args.backbone)
        config_path = os.path.join(model_dir, "config.json")
        with open(config_path, 'w') as f:
            json.dump(config, f, indent=2)
        print(f"Generated {config_path}")

        # Convert to ONNX
        onnx_path = os.path.join(model_dir, "model.onnx")
        if args.backbone == 'mobilenet':
            success = convert_mobilenet_to_onnx(args.weights, onnx_path)
        else:
            success = convert_inception_resnet_to_onnx(args.weights, onnx_path)

        if not success:
            sys.exit(1)

        # Package into .dtmodel
        dtmodel_path = output_dir / f"{model_id}.dtmodel"
        package_dtmodel(model_dir, str(dtmodel_path))

        # Also copy extracted model for direct use
        extracted_dir = output_dir / model_id
        if extracted_dir.exists():
            shutil.rmtree(extracted_dir)
        shutil.copytree(model_dir, extracted_dir)
        print(f"Extracted model copied to {extracted_dir}")

    print("\nDone! To install the model:")
    print(f"  cp {dtmodel_path} <darktable-models-dir>/")
    print("Or for built-in bundling, place the extracted folder in data/models/")


if __name__ == '__main__':
    main()
