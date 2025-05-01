import sys
import hashlib
import subprocess

from elftools.elf.elffile import ELFFile


def process_file(filename):
    print('In file:', filename)
    with open(filename, 'rb') as f:
        elffile = ELFFile(f)

        sections = []

        for section in elffile.iter_sections():
            if section.name.startswith('.overlay'):
                if (len(section.data()) != section.data_size):
                    print(section.name, len(section.data()), section.data_size)
                sections.append(section)

        overlay_size = max([section.data_size for section in sections])

        for section in sections:
            to_hash = section.data()
            if len(to_hash) < overlay_size:
                to_hash += b'\x00' * (overlay_size - len(to_hash))
            hash = hashlib.sha256(to_hash).hexdigest()
            name = section.name.replace('.overlay_', '')
            wsl_filename = filename.replace("C:/", "/mnt/c/")
            subprocess.run(['wsl', 'picotool', 'seal', '--quiet', wsl_filename, wsl_filename], check=True)
            subprocess.run(['wsl', 'picotool', 'config', '-s', f'{name}_hash', hash, wsl_filename], check=True)


if __name__ == '__main__':
    for filename in sys.argv[1:]:
        process_file(filename)
