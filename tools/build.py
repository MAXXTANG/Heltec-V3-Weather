"""Build the WiFi LoRa 32 V3 OLED project using the local toolchain cache."""
from pathlib import Path
import os, sys
ROOT = Path(__file__).resolve().parents[1]
os.environ['PLATFORMIO_CORE_DIR'] = str(ROOT / '.pio-core')
os.environ['PLATFORMIO_SETTING_ENABLE_TELEMETRY'] = 'no'
from platformio.platform.base import PlatformBase
original = PlatformBase.configure_project_packages
def configure(self, env, targets=None):
    original(self, env, targets)
    assert env == 'heltec-v3'
    self.packages['toolchain-riscv32-esp']['optional'] = True
PlatformBase.configure_project_packages = configure
from platformio.__main__ import main
sys.argv = ['platformio', 'run', '--project-dir', str(ROOT)] + sys.argv[1:]
raise SystemExit(main())
