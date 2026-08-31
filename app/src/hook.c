
extern int __real_main(void);
void fi_msgq_report(void);
void fi_sem_report(void);


int __wrap_main(void){
    int ret = __real_main();
    fi_msgq_report();
    fi_sem_report();

    

    return ret;
}