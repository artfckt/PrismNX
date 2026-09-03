"""Apply narrowly scoped libtesla lifecycle guards to a build-local copy."""
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
source = ROOT / "third_party/fizeau/lib/libtesla/include"
target = ROOT / "build/tesla/include"
shutil.copytree(source, target, dirs_exist_ok=True)
header = target / "tesla.hpp"
text = header.read_text(encoding="utf-8")
old = "if (this->m_guiStack.top() != nullptr && this->m_guiStack.top()->m_focusedElement != nullptr)"
assert text.count(old) == 1
text = text.replace(old, "if (!this->m_guiStack.empty() && this->m_guiStack.top() != nullptr && this->m_guiStack.top()->m_focusedElement != nullptr)")
old = """                    if (keysDown & HidNpadButton_B)
                        this->goBack();"""
assert text.count(old) == 1
text = text.replace(old, """                    if (keysDown & HidNpadButton_B) {
                        this->goBack();
                        return; // The former currentGui reference may now be invalid.
                    }""")
header.write_text(text, encoding="utf-8", newline="\n")
print("Prepared libtesla with empty-stack and back-navigation guards")
