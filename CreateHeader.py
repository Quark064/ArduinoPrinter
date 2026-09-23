# pip install Pillow
from PIL import Image

from sys import argv

IMAGE_PATH = "test.bmp"
HEADER_OUTPUT_PATH = "."

INT_SIZE = 8

def main():
    if (len(argv) <= 1):
        print("No input image path argument was provided.")
        return
    
    img = Image.open(argv[1]).convert("1")

    width, height = img.size
    if width != 320 or height != 120:
        print(f"Image was not the correct size. Expected 320 x 120, got {width} x {height}")
        return

    pixels = list(img.get_flattened_data()) # type: ignore
    
    packedArray = []
    for i in range(0, len(pixels), INT_SIZE):
        chunk = [0 if x == 255 else 1 for x in pixels[i:i+INT_SIZE]]
        
        byte = 0
        for bit in chunk:
            byte = (byte << 1) | bit
        
        packedArray.append(byte)
    
    headerFile = [
        "#include <stdint.h>",
        "#include <avr/pgmspace.h>",
        "",
        f"const uint{INT_SIZE}_t PAYLOAD[{len(packedArray)}] PROGMEM = {{ {",".join(map(str, packedArray))} }};"
    ]

    with open(f"{HEADER_OUTPUT_PATH}/Payload.h", "w") as file:
        file.write("\n".join(headerFile))
    


if __name__ == "__main__":
    main()
