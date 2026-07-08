#!/usr/bin/env python3
"""Deep learning model file analysis (PyTorch, ONNX, safetensors, HDF5, TFLite)."""

import sys
import os
import struct
import json

from _common import *

ONNX_DTYPE_MAP = {}

def _load_onnx_dtype_map():
    try:
        from onnx import TensorProto
        for name in dir(TensorProto):
            val = getattr(TensorProto, name)
            if isinstance(val, int) and name.isupper() and not name.startswith('_'):
                ONNX_DTYPE_MAP[val] = name
    except Exception:
        pass

def dumptype(v):
    return ONNX_DTYPE_MAP.get(v, f"UNKNOWN({v})")


# ─── ONNX ───────────────────────────────────────────────────────────

def parse_onnx(path, width):
    import onnx
    try:
        model = onnx.load(path)
    except Exception as e:
        return False

    section("ONNX Model")
    label_value("Producer", f"{model.producer_name or '?'} {model.producer_version or ''}")
    label_value("IR Version", str(model.ir_version))
    label_value("Model Version", str(model.model_version) if model.HasField('model_version') else "N/A")
    label_value("Domain", model.domain or "N/A")

    if model.doc_string:
        label_value("Doc", model.doc_string.strip()[:200])

    graph = model.graph
    if graph.name:
        label_value("Graph", graph.name)

    section("Inputs")
    for inp in graph.input:
        shape = [str(d.dim_value) if d.dim_value > 0 else '?' for d in inp.type.tensor_type.shape.dim]
        dtype = dumptype(inp.type.tensor_type.elem_type)
        print(f"  {Y}Name{R}: {inp.name}  {Y}Shape{R}: ({', '.join(shape)})  {Y}Type{R}: {dtype}")

    section("Outputs")
    for out in graph.output:
        shape = [str(d.dim_value) if d.dim_value > 0 else '?' for d in out.type.tensor_type.shape.dim]
        dtype = dumptype(out.type.tensor_type.elem_type)
        print(f"  {Y}Name{R}: {out.name}  {Y}Shape{R}: ({', '.join(shape)})  {Y}Type{R}: {dtype}")

    nodes = list(graph.node)
    print(f"\n  {Y}Nodes{R}: {len(nodes)}")

    op_types = {}
    for n in nodes:
        op_types[n.op_type] = op_types.get(n.op_type, 0) + 1
    if op_types:
        section("Operators")
        for op, cnt in sorted(op_types.items(), key=lambda x: -x[1]):
            print(f"  {G}{op:>20}{R}  x{cnt}")

    section("Value Info")
    val_count = len(graph.value_info)
    print(f"  {G}{val_count}{R} intermediate tensors")

    initializer_count = len(graph.initializer)
    total_params = sum(
        int(pd.dim_value) for init in graph.initializer
        for pd in init.dims
    )
    if initializer_count > 0:
        print(f"  {G}{initializer_count}{R} initializers  {G}{total_params:,}{R} parameters")

    return True


# ─── PyTorch ────────────────────────────────────────────────────────

def parse_torch(path, width):
    import torch
    try:
        data = torch.load(path, map_location='cpu', weights_only=True)
    except Exception as e:
        return False

    section("PyTorch Model")

    if isinstance(data, dict):
        label_value("Type", "state_dict (dict)")

        sd = data.get('state_dict') or data.get('model_state_dict') or data

        if isinstance(sd, dict):
            tensor_keys = [(k, v) for k, v in sd.items() if hasattr(v, 'shape')]
            total_params = 0

            print(f"\n  {B}{'Tensor':<50} {'Shape':<30} {'Dtype':<10}{R}")
            print(f"  {D}{'-'*50} {'-'*30} {'-'*10}{R}")

            for k, v in tensor_keys[:40]:
                shape_str = str(list(v.shape)) if hasattr(v, 'shape') else '?'
                print(f"  {W}{k:<50}{R} {W}{shape_str:<30}{R} {C}{str(v.dtype):<10}{R}")
                if hasattr(v, 'numel'):
                    total_params += v.numel()
                elif hasattr(v, 'shape'):
                    import math
                    total_params += math.prod(v.shape) if v.shape else 0

            if len(tensor_keys) > 40:
                print(f"  {D}... and {len(tensor_keys) - 40} more tensors{R}")

            if total_params > 0:
                label_value("Total Parameters", f"{total_params:,}")

        if 'epoch' in data:
            label_value("Epoch", str(data['epoch']))
        if 'optimizer' in data or 'optimizer_state_dict' in data:
            label_value("Has Optimizer", "yes")
        if 'best_acc' in data or 'best_loss' in data:
            label_value("Best Acc", str(data.get('best_acc', '')))
            label_value("Best Loss", str(data.get('best_loss', '')))

    elif hasattr(data, '_modules'):
        label_value("Type", "nn.Module")
        label_value("Class", type(data).__name__)
        params = sum(p.numel() for p in data.parameters())
        label_value("Parameters", f"{params:,}")
        trainable = sum(p.numel() for p in data.parameters() if p.requires_grad)
        label_value("Trainable", f"{trainable:,}")
    else:
        label_value("Type", type(data).__name__)
        if hasattr(data, 'shape'):
            label_value("Shape", str(list(data.shape)))

    return True


# ─── Safetensors ───────────────────────────────────────────────────

def parse_safetensors(path, width):
    import safetensors
    try:
        with safetensors.safe_open(path, framework="pt") as f:
            keys = f.keys()
    except Exception as e:
        return False

    section("Safetensors")

    metadata = {}
    try:
        with open(path, 'rb') as fh:
            header_len = struct.unpack('<Q', fh.read(8))[0]
            header_json = json.loads(fh.read(header_len))
            metadata = header_json.get('__metadata__', {})
    except Exception:
        pass

    if metadata:
        for k, v in metadata.items():
            label_value(k.title(), str(v)[:200])

    tensor_count = len(keys)
    total_params = 0
    print(f"\n  {B}{'Tensor':<50} {'Shape':<30} {'Dtype':<10}{R}")
    print(f"  {D}{'-'*50} {'-'*30} {'-'*10}{R}")

    for k in keys[:40]:
        tensor = f.get_tensor(k)
        shape_str = str(list(tensor.shape))
        print(f"  {W}{k:<50}{R} {W}{shape_str:<30}{R} {C}{str(tensor.dtype):<10}{R}")
        total_params += tensor.numel()

    if tensor_count > 40:
        print(f"  {D}... and {tensor_count - 40} more tensors{R}")

    label_value("Tensors", str(tensor_count))
    if total_params > 0:
        label_value("Total Parameters", f"{total_params:,}")

    return True


# ─── HDF5 / Keras ──────────────────────────────────────────────────

def parse_h5(path, width):
    import h5py
    try:
        f = h5py.File(path, 'r')
    except Exception as e:
        return False

    section("HDF5 / Keras Model")

    items = []

    def walk(name, obj):
        if isinstance(obj, h5py.Dataset):
            shape = str(list(obj.shape))
            dtype = str(obj.dtype)
            items.append((name, shape, dtype, False))
        elif isinstance(obj, h5py.Group):
            items.append((name, '', '', True))

    f.visititems(walk)

    max_show = 50
    shown = 0
    for name, shape, dtype, is_group in items[:max_show]:
        if is_group:
            print(f"  {C}{name}/{R}")
        else:
            print(f"  {W}{name:<50}{R} {W}{shape:<25}{R} {C}{dtype}{R}")

    if len(items) > max_show:
        print(f"  {D}... and {len(items) - max_show} more items{R}")
    else:
        if not items:
            print(f"  {D}(empty file){R}")

    f.close()
    return True


# ─── TFLite ─────────────────────────────────────────────────────────

def parse_tflite(path, width):
    try:
        import tensorflow as tf
        interpreter = tf.lite.Interpreter(model_path=path)
        interpreter.allocate_tensors()
    except Exception as e:
        return False

    section("TFLite Model")

    details = interpreter.get_tensor_details()
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    label_value("Input Tensors", str(len(input_details)))
    for d in input_details:
        name = d.get('name', '?')
        shape = str(list(d['shape']))
        dtype = str(d['dtype'])[8:-2] if str(d['dtype']).startswith('<class') else str(d['dtype'])
        quant = d.get('quantization', None)
        qstr = f" quant={quant}" if quant and quant != (0.0, 0) else ""
        print(f"  {Y}Name{R}: {name}  {Y}Shape{R}: {shape}  {Y}Type{R}: {dtype}{qstr}")

    label_value("Output Tensors", str(len(output_details)))
    for d in output_details:
        name = d.get('name', '?')
        shape = str(list(d['shape']))
        dtype = str(d['dtype'])[8:-2] if str(d['dtype']).startswith('<class') else str(d['dtype'])
        print(f"  {Y}Name{R}: {name}  {Y}Shape{R}: {shape}  {Y}Type{R}: {dtype}")

    label_value("Total Tensors", str(len(details)))
    subgraphs = interpreter._interpreter.SubgraphsSize() if hasattr(interpreter._interpreter, 'SubgraphsSize') else 1
    label_value("Subgraphs", str(subgraphs))

    try:
        desc = interpreter.get_model_details()
        if desc:
            label_value("Model Version", str(desc.get('model_version', 'N/A')))
    except Exception:
        pass

    return True


# ─── Main ───────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 2:
        print("Usage: dl_parser.py <model_file> [width]", file=sys.stderr)
        sys.exit(1)

    path = sys.argv[1]
    width = int(sys.argv[2]) if len(sys.argv) > 2 else 80

    if not os.path.isfile(path):
        print(f"{R}File not found: {path}{R}")
        sys.exit(1)

    ext = os.path.splitext(path)[1].lower()

    _load_onnx_dtype_map()

    print(f"\n{B}[ Deep Learning Model Analysis ]{R}")
    label_value("File", os.path.basename(path))
    label_value("Size", fmt_size(os.path.getsize(path)))

    parsed = False

    if ext == '.onnx':
        parsed = parse_onnx(path, width)
    elif ext in ('.pt', '.pth'):
        parsed = parse_torch(path, width)
    elif ext == '.safetensors':
        parsed = parse_safetensors(path, width)
    elif ext in ('.h5', '.hdf5', '.keras'):
        parsed = parse_h5(path, width)
    elif ext == '.tflite':
        parsed = parse_tflite(path, width)

    if not parsed:
        label_value("Type", try_file(path))

        with open(path, 'rb') as fh:
            raw = fh.read(64)
        print()
        section("File Header Hex")
        hex_dump(raw, 0, width)
        print()

    print()


if __name__ == "__main__":
    main()
