all: plant assembly_manager

plant: plant_manager.c
	gcc -o plant plant_manager.c -pthread -Wall -Werror

assembly_manager: assembly_manager.c
	gcc -o assembly_manager assembly_manager.c -pthread -Wall -Werror

clean:
	rm -f plant assembly_manager *.o