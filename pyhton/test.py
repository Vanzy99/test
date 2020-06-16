import sys
print('Enter a number: ')
years = int(sys.stdin.readline())


if ((years % 4 == 0) and (years % 100 != 0)) or (years % 400 == 0):
	print(years, ' is runnian ')

