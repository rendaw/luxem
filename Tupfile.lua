local sources = tup.glob('*.c')
tup.definerule{
	inputs = sources,
	outputs = {'a.out'},
	command = 'clang -ansi -pedantic -Wall -Werror -ggdb -O0 ' .. table.concat(sources, ' ')
}
