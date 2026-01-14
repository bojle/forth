import subprocess, re, sys

def test(file):
    with open(file) as f:
        head = f.readline()
        body = f.read()

    expect = re.search(r'\((.*?)\)', head).group(1).strip()
    
    proc = subprocess.run(['./forth'], input=body, text=True, capture_output=True)
    actual = proc.stdout.strip()

    if expect == actual:
        print(f"OK: {file}")
    else:
        print(f"FAIL: {file} | Expected [{expect}] got [{actual}]")

if __name__ == "__main__":
    for arg in sys.argv[1:]:
        test(arg)
