#!/usr/bin/env python3

def main(count=10000):
    f = open("requests/t1.toml", "w")
    for i in range(1, count + 1):
        f.write("""[[events]]
type = \"CREATE\"
method = \"GET\"
uri = \"README.md\"
id = {0}

[[events]]
type = \"SEND_ALL\"
id = {0}

""".format(i))
    for i in range(1, count + 1):
        f.write("""[[events]]
type = \"WAIT\"
id = {}

""".format(i))
    f.close()

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        main(int(sys.argv[1]))
    else:
        main()
