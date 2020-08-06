import argparse
import csv
import pathlib
import shutil
import sys
from pathlib import Path
from typing import List

from PIL import Image, ImageDraw
from tqdm import tqdm


def parse_args():
    parser = argparse.ArgumentParser(
        description="Given a series of images and a list of robustness values, annotate each image"
    )
    parser.add_argument("robfile", type=lambda p: Path(p).absolute())
    parser.add_argument("imgfiles", nargs="+")
    parser.add_argument(
        "-o",
        "--output-dir",
        type=lambda p: Path(p).absolute(),
        default=Path(__file__).absolute().parent / "annotations",
    )

    return parser.parse_args()


def annotate(robustness: bool, imgpath: Path, outpath: Path):
    color = (255, 0, 0) if not robustness else (0, 255, 0)
    # Copy original img
    #  shutil.copy(imgpath, outpath)
    with Image.open(imgpath) as im:
        draw = ImageDraw.Draw(im)
        draw.rectangle((100, 100, 200, 200), fill=color, outline=(255, 255, 255))
        im.save(outpath)


def main():
    args = parse_args()
    # Check if robustness exists
    robfile: Path = args.robfile
    if not robfile.is_file():
        sys.exit(f"Robustness file {robfile} does not exist")
    # Load robustness into list
    robustness = []
    with open(robfile, "r") as input_robustness:
        robreader = csv.reader(input_robustness)
        for row in robreader:
            frame_num = int(row[0].strip())
            sat_val = row[1].strip().lower() in ["true", "1"]
            robustness.append(sat_val)

    # Check if each image exists
    img_files: List[Path] = []
    for img in args.imgfiles:
        i = Path(img).absolute()
        if not i.is_file():
            sys.exit(f"{i} is an incorrect image file")
        else:
            img_files.append(i)

    # Check if number of img files == number of robustness vals
    if len(img_files) != len(robustness):
        sys.exit("Number of frames in robustness != number of given images")

    # Make output path
    outdir = Path(args.output_dir)
    outdir.mkdir(parents=True, exist_ok=True)
    print(f"Outdir : {outdir}")

    # Annotate imgs
    for rob, img in tqdm(zip(robustness, img_files), total=len(robustness)):
        # Set name for output img
        img_name = img.name
        out_name = outdir / img_name
        annotate(rob, img, out_name)


if __name__ == "__main__":
    main()
