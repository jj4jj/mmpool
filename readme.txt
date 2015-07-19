

fixed size memory pool .
	alloc
		find a free block
	free
		set the free block used
		

	bitmap .
		usage rate is low , block_size*N	-> 		N/8 Bytes
				block_size * 8		-> block_size = 16 =>	128	8/1000,	64 => 512 
				2/1000
				1 / (block_size * 8 + 1)		
	blocklist
		free	-> A B C
		using	-> D + A
				block_size / 8		-> block_size = 16 =>	30 / 100 ,	64 => 
				1/9
				1 / (block_size / 8 + 1)

flexiable size memory pool
	link list 
	|-------------------------------------------|	1024
	|------|16|---------------------------------|
	used	|----16--|-----8----|------8---|
	free	|----------992-----|

	used	|---16--| ------ | ---8 ----|
	free 	|---8----| ------992 ----|
	O(n) -> O(logN)

	according the block size alloc list (block_size)
	1
	2
	4
	8
	16
	32
	the list should be short
	merging the small block

	alloc	N
		1. 	get smallest n >= N block list
		2.	if not found , merging to n.
		3.  if found , set one alloced
	

	buddy system 			

		














