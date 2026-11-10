import sys
from pathlib import Path

# Make the project root importable regardless of how pytest is invoked.
ROOT = Path(__file__).parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
