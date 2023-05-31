import sys;
print(sys.argv);

nums = sys.argv[1].split(".");
print(nums);

res = 0;

for i, x in enumerate(nums):
    res += int(x);
    if (i != 3):
        res <<= 8;

print(hex(res));
