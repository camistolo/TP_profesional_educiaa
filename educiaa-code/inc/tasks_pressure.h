#ifndef TASKS_H
#define TASKS_H

#define MAX_COL 14
#define MAX_ROW 14

void set_matrix_index( void* pvParameters );
void get_pressure_value( void* pvParameters );
void print_matrix( void* pvParameters );


void SetMuxChannel(int channel);
void SetDeMuxChannel(int channel);

#endif //TASKS_H
