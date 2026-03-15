from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
PROTO_ROOT = REPO_ROOT / "proto"
GEN_ROOT = REPO_ROOT / "tests" / ".gen_py"

PROTO_FILES = [
    PROTO_ROOT / "kvd" / "options.proto",
    PROTO_ROOT / "kvd" / "api" / "v1" / "envelope.proto",
    PROTO_ROOT / "kvd" / "api" / "v1" / "kv.proto",
    PROTO_ROOT / "kvd" / "api" / "v1" / "numeric.proto",
    PROTO_ROOT / "kvd" / "api" / "v1" / "ttl.proto",
]


def ensure_python_protos() -> None:
    required = [
        GEN_ROOT / "kvd" / "api" / "v1" / "envelope_pb2.py",
        GEN_ROOT / "kvd" / "api" / "v1" / "kv_pb2.py",
        GEN_ROOT / "kvd" / "api" / "v1" / "numeric_pb2.py",
        GEN_ROOT / "kvd" / "api" / "v1" / "ttl_pb2.py",
    ]

    if all(p.exists() for p in required):
        if str(GEN_ROOT) not in sys.path:
            sys.path.insert(0, str(GEN_ROOT))
        return

    protoc = shutil.which("protoc")
    if protoc is None:
        raise RuntimeError("protoc not found in PATH")

    GEN_ROOT.mkdir(parents=True, exist_ok=True)

    cmd = [
        protoc,
        f"--proto_path={PROTO_ROOT}",
        f"--python_out={GEN_ROOT}",
        *(str(p.relative_to(PROTO_ROOT)) for p in PROTO_FILES),
    ]
    subprocess.run(cmd, check=True)

    if str(GEN_ROOT) not in sys.path:
        sys.path.insert(0, str(GEN_ROOT))


ensure_python_protos()

from kvd.api.v1 import envelope_pb2, kv_pb2, numeric_pb2, ttl_pb2  # noqa: E402