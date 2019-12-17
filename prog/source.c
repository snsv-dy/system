
#define SYSCALL_PUTS 0
#define SYSCALL_GETS 1
#define SYSCALL_SWITCH_TASK 2
#define SYSCALL_SLEEP 3

extern void syscall(unsigned int call_type, void *data);

void puts(char *s){
	syscall(SYSCALL_PUTS, s);
}

void yeld(){
	syscall(SYSCALL_SWITCH_TASK, 0);
}

#define SLEEP_SHORTEST 10
#define CEIL(a, b) (((a) + (b) - 1) / (b))

// void msleep(int msecs){
// 	// int num_interrupts = CEIL(msecs, SLEEP_TICK_SPAN);
// 	int num_interrupts = CEIL(msecs, SLEEP_SHORTEST);
// 	syscall(SYSCALL_SLEEP, num_interrupts);
// }


void sleep(int duration){
	// duration = 
	syscall(SYSCALL_SLEEP, duration);
}

#define msleep(x) sleep(CEIL(x, SLEEP_SHORTEST))

// program w formie którą może odczytać człowiek
// w drugim wątku jest zmieniony tekst cosnt na Thread2, i wartość w msleep na 1000

int main(){

	char arr[50];
	char *cosnt = "Thread1";
	for(int i = 0; i < 7; i++)
		arr[i] = cosnt[i];
	arr[7] = '\0';
	int k = 0;

	while(1){
		arr[7] = ' ';
		arr[8] = k / 10+ '0';
		arr[9] = k % 10+ '0';
		arr[10] = '\0';
		k++;

		puts(arr);
		msleep(400);
	}

	return 0;
}