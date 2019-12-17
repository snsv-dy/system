
f = open("myos.bin", "rb")

f.seek(0, 2)
fsize = f.tell()
f.seek(0, 0)
print(fsize)
csize = fsize
if fsize % 4 != 0:
	csize += 4 - (fsize % 4)

data = []
for i in range(0, csize, 4):
	rd = f.read(4)
	data.append(int.from_bytes(rd, byteorder='little'))

print("Data: ")
for i in data:
	print(i)
print("")

pstr = "{"
for i in range(len(data)):
	pstr += '0x{:02x}'.format(data[i])
	if(i != len(data) - 1):
		pstr += ', '
pstr += "};"

out = open("result.txt", "w")
out.write(pstr)

print(pstr)
