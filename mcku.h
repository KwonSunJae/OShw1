#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//8bit 주소체계
//PFN 6bit
//Offset 4bit
//Linear Page Table
//PTE { PFN, Unusedbit, Present bit}로 구성
//RoundRobin Scheduling
//Page size: 16Byte
//proc.txt는 VA를 조회해서 PA에 저장된값을 가져올것임.

extern struct pcb * current;
extern char * ptbr;
struct pcb{
	char pid;
	FILE *fd;
	char *pgtable;
	/* Add more fields if needed */
	struct pcb * next;
};
struct array_pcb{
	struct pcb * start;
	int cnt;
	int rr;
};



struct array_pcb pl = {NULL, 0,0};

int FreeList[64];

void append(struct pcb * p){
	if (pl.cnt==0){
		pl.start=p;
		pl.cnt++;
		return;
	}
	struct pcb * temp = pl.start;
	while(temp->next!= NULL){
		temp = temp->next;
	}
	temp->next = p;
	pl.cnt ++;
	
}

struct pcb * create_pcb(char index,char * fileName){
	if(index == -1){
		struct pcb * end = (struct pcb *) malloc (sizeof (struct pcb));
		end->pid =0;
		return end;
	}
	struct pcb * temp = (struct pcb * )malloc(sizeof(struct pcb));
	
	temp->pgtable = (char*)malloc(sizeof(char)*16);
	
	memset(temp->pgtable, 0,sizeof(char)*16);
	
	temp->pid = index;
	temp->next = NULL;
	char *f = strtok(fileName,"\n");
	temp->fd = fopen(f,"r");
	return temp;
}

void delete_pcb(char pids){
	
	struct pcb * temp = pl.start;

	if(pl.cnt == 0) {
		free(current);
		return;
	}
	if(pl.cnt == 1) {
		
		for(int i =0; i<16; i++){
			char pte = current->pgtable[i];
		
			if(pte == 0)continue;
			int remove = (pte & 0b11111100) >> 2;
			FreeList[remove] =0;
		}
		
		free(current->pgtable);
		fclose(current->fd);
		
		free(current);
		
		current = create_pcb(-1, NULL);
		pl.cnt = 0;
		pl.rr = 0;
		return;
	}
	
	if(temp->pid == pids){
		
		for(int i =0; i<16; i++){
			char pte = current->pgtable[i];
		
			if(pte == 0)continue;
			int remove = (pte & 0b11111100) >> 2;
			FreeList[remove] =0;
		}
		free(temp->pgtable);
		fclose(temp->fd);
		free(temp);
		pl.start = temp->next;
		pl.cnt --;
		pl.rr = (pl.rr-1)%(pl.cnt);
		return;
	}
	else{
		while(temp->next->pid != pids){
			
			temp = temp->next;
		}
	}
	
	
	struct pcb * cur = temp->next->next;
	
	for(int i =0; i<16; i++){
		char pte = current->pgtable[i];
		
		if(pte == 0)continue;
		int remove = (pte & 0b11111100) >> 2;
		FreeList[remove] =0;
	}
	
	free(temp->next->pgtable);
	fclose(temp->next->fd);
	free(temp->next);
	
	temp->next = cur;
	pl.cnt --;
	
	pl.rr = (pl.rr-1)%(pl.cnt);
	
	
}
void ku_scheduler(char pid){
	//현재 실행되고있는 process의 pcb랑 커렌트 설정을 잘해줘야함.
	/* Your code here */
	if(pl.cnt == 0) {free(current); current= NULL; return;}
	
	struct pcb * sel = pl.start;
	pl.rr = (pl.rr+1)%(pl.cnt);
	for(int i =0; i<pl.rr; i++){
		if(pl.cnt == 1 ) break;
		sel = sel->next;
	}
	
	current = sel;
	ptbr = sel->pgtable;
	
}


void ku_pgfault_handler(char va){
	//pagefault가 발생했을때 Freelist를 생성및 관리 참고해서 pgtable에 등록해줘야함. (64 * 16byte 메모리 할당할 필요는 없음.Free list는 64page Frames임.)
	/* Your code here */
	int i;
	int flag = 0;
	for(i=0; i<64; i++){
		if(FreeList[i]==0){flag =1;break;}
	}
	
	char * pte;
	int ptIndex = (va & 0xF0 ) >> 4;
	pte = ptbr + ptIndex;
	if(!flag){*pte = 0;return;}
	int pa = i << 2;
	*pte = pa + 2 + 1;
	FreeList[i] = 1;
}


void ku_proc_exit(char pid){
	//해당 종료할 프로세스 아이디를 바탕으로 프로세스를 종료시켜야함. 모든 자원들을 리핑 해줘야함. pcb, PageFrames mapped >> Update the free list 해줘야함.
	/* Your code here */
	
	
	delete_pcb((char)pid);
}


void ku_proc_init(int nprocs, char *flist){
	//process들의 pcb를 초기화해줘야함.
	/* Your code here */
	FILE * fl = fopen(flist,"r");

	for (int i =0; i<nprocs; i++){
		char buf[1024];
		fgets( buf, sizeof( buf), fl);
		
		append(create_pcb(i,buf));
	}
	current = pl.start;
	ptbr = current->pgtable;
	fclose(fl);
}