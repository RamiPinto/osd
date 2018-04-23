#include <stdio.h>
#include <limits.h>

int cceil(double x){
	int y = (int) x, result;
	if((x - y) > 0){
		result = y+1;
	}
	else{
		result = y;
	}

	return result;
}

int main() {
   printf("Storage size for int : %d \n", (int) sizeof(int));

   printf("Ceil of 23.4 is %d\n", cceil(23.4));
   printf("Ceil of 0 is %d\n", cceil(0.0));
   printf("Ceil of 10 is %d\n", cceil(10.0));

      return 0;

}
