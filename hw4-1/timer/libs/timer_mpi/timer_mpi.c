#include"timer_mpi.h"

Timer::Timer(){
    this->io = 0;
    this->cpu = 0;
    this->comm = 0;
    this->is_enable_mpi = 0;
    this->is_enable = 1;
    this->unit = SEC;
    this->rank = 0;
    this->c_size = 0;
    this->show_rank = 0;
    this->is_enable_show = 1;
}

Timer::~Timer(){
}

void Timer::start_rec(const char *s){
    if(!this->is_enable){return;}
    clock_gettime(CLOCK_MONOTONIC, &(this->work[s]));

    // this->work[s] = clock();
}

double Timer::pause_rec(const char *s){
    if(!this->is_enable){return 0;}

    // double accum = this->rec[s];
    // clock_t start = this->work[s];
    // clock_t end = clock();
    // accum += this->convert(start, end);
    // this->rec[s] = accum;

    struct timespec end, temp, start = this->work[s];
    double accum = this->rec[s];
    clock_gettime(CLOCK_MONOTONIC, &(end));
    
    // temp.tv_sec = end.tv_sec - start.tv_sec;
    // temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    // accum += temp.tv_sec + (double) temp.tv_nsec / 1000000000.0;
    accum += this->convert(start, end);

    this->rec[s] = accum;

    return accum;
}

double Timer::get_rec(const char *s){
    if(!this->is_enable){return 0;}

    return this->rec[s];
}

void Timer::set_rec(const char *s, double t){
    this->rec[s] = t;
}

void Timer::clear_rec(const char *s){
    if(!this->is_enable){return;}

    this->rec[s] = 0;
}

void Timer::show_rec(const char *s){
    if((!this->is_enable) || (!this->is_enable_show)){return;}

    if(this->is_enable_mpi){
        if(this->rank == this->show_rank){
            printf("Rec[%s] \t %lf %c\n", s, this->rec[s], this->unit);
        }
    }else{
        printf("Rec[%s] \t %lf %c\n", s, this->rec[s], this->unit);
    }
    
}

bool Timer::is_in(const char *s){
    if(!this->is_enable){return false;}

    if ( this->rec.find(s) == this->rec.end() ) {return false;}
    else {return true;}
}

double Timer::convert(struct timespec s, struct timespec e){
    if(!this->is_enable){return 0;}
    struct timespec temp;
    temp.tv_sec = e.tv_sec - s.tv_sec;
    temp.tv_nsec = e.tv_nsec - s.tv_nsec;
    // accum += temp.tv_sec + (double) temp.tv_nsec / 1000000000.0;

    switch (this->unit){
    case MSC:
        // return ((double) (e - s)) * 1000 / CLOCKS_PER_SEC;
        return (double) temp.tv_nsec / 1000000.0;
        break;
    case SEC:
        // return ((double) (e - s)) / CLOCKS_PER_SEC;
        return temp.tv_sec + (double) temp.tv_nsec / 1000000000.0;
        break;
    // case CLK:
    //     return (double) (e - s);
    //     break;
    default:
        return temp.tv_sec + (double) temp.tv_nsec / 1000000000.0;
        break;
    }
    
}

void Timer::report(const char* f_n, const char **order, int n){
    if(!this->is_enable){return;}

    if(this->is_enable_mpi){
        if(this->rank != this->show_rank){return;}
    }

    FILE *fp;

    fp = fopen(f_n, "w+");
    for(int i = 0; i < n; i++){
        // printf("%s, ", order[i]);
        fprintf(fp, "%s, ", order[i]);
    }
    // printf("\n");
    fprintf(fp, "\n");
    for(int i = 0; i < n; i++){
        printf("%lf, ", this->rec[order[i]]);
        fprintf(fp, "%lf, ", this->rec[order[i]]);
    }
    // printf("\n");
    fprintf(fp, "\n");

    fclose(fp);
}

double Timer::reduce(double t){
    if(!this->is_enable){return 0;}

    double time = t, sum = 0;
    if(this->is_enable_mpi){MPI_Allreduce(&time, &sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);}
    else{sum = t;}
    
    return sum;
}

void Timer::reduce_rec(const char *s){
    if(!this->is_enable){return;}

    if(this->is_enable_mpi){
        double time = this->rec[s], sum = 0;
        MPI_Allreduce(&time, &sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        this->rec[s] = sum;
    }
}

void Timer::reduce_all(){
    if(!this->is_enable){return;}

    if(this->is_enable_mpi){
        for(std::map<const char*, double>::iterator it = this->rec.begin(); it != rec.end(); it++){
            double time = it->second, sum = 0;
            MPI_Allreduce(&time, &sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
            it->second = sum;
        }  
    }
}

Timer* Timer::set_unit(char c){this->unit = c; return this;}
Timer* Timer::set_mpi_rank(int rank){this->rank = rank; return this;}
Timer* Timer::set_mpi_comm_size(int size){this->c_size = size; return this;}
Timer* Timer::set_mpi_show_rank(int rank){this->show_rank = rank; return this;}
Timer* Timer::enable_mpi(){this->is_enable_mpi = 1; return this;}
Timer* Timer::enable(){this->is_enable = 1; return this;}
Timer* Timer::disable(){this->is_enable = 0; return this;}
Timer* Timer::disable_show(){this->is_enable_show = 0; return this;}
Timer* Timer::enable_show(){this->is_enable_show = 1; return this;}
