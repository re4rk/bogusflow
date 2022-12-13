#include<stdio.h>
#include<stdlib.h>
#include<time.h>

void initRandom(){
    srand(time(NULL));
}

void generateRandomNumbers(int *comp){
    int i,j;
    for(i=0;i<3;i++){
        comp[i]=rand()%9+1;
        for(j=0;j<i;j++){
            if(comp[i]==comp[j]){
                i--;
                break;
            }
        }
    }
}

void getUserInput(int *user){
    int i;
    char temp[3];
    int pass = 0;
    while(pass==0){
        pass = 1;
        scanf("%s",temp);
        for(i=0;i<3;i++){
            user[i]= temp[i] - '0';
        }

        for(i=0;i<3;i++){
            if(user[i]<1 || user[i]>9){
                printf("Invalid input. Try again.\n");
                pass = 0;
                break;
            }
        }
    }
}

int compare(int *user, int *comp){
    int i,j;
    int strike=0;
    int ball=0;
    for(i=0;i<3;i++){
        for(j=0;j<3;j++){
            if(user[i]==comp[j]){
                if(i==j){
                    strike++;
                }else{
                    ball++;
                }
            }
        }
    }
    printf("%d strike(s), %d ball(s)\n",strike,ball);
    if(strike==3){
        printf("You win!\n");
        return 1;
    }
    return 0;
}


int main(void){
    int user[3];
    int comp[3];
    int chance=10;
    initRandom();
    generateRandomNumbers(comp);
    while (chance > 0) {
        // notice chance
        printf("Chance left: %d\n", chance);
        printf("Enter 3 numbers: ");
        getUserInput(user);
        if (compare(user,comp)) {
            break;
        }
        chance--;
    }
    return 0;
}