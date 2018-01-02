// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-cam.cpp
// Implementación del problema de los filósofos (con camarero).
// 
// -----------------------------------------------------------------------------


#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
num_filosofos = 5 ,
  num_procesos  = 2*num_filosofos + 1,
  etiq_pedir_asiento = 1,
  etiq_liberar_asiento = 2;


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

// ---------------------------------------------------------------------

void funcion_filosofos( int id )
{
  int id_ten_izq = (id+1) % num_procesos, //id. tenedor izq.
    id_ten_der = (id-1); //id. tenedor der.

  if(id_ten_izq == 0) id_ten_izq = 1; // En caso de que sea el último filósofo, ya no corresponde la id con la que proporcionaba el módulo
  
  int id_camarero = 0;

  int aux = 0; // No es usado, pero es necesario algún buffer para transmitir los mensajes
  while ( true )
    {
      // Se añade a las peticiones anteriores, la petición de sentarse
      cout << "Filósofo " << id << " solicita sentarse " << endl;
      MPI_Ssend(&aux, 1, MPI_INT, id_camarero, etiq_pedir_asiento, MPI_COMM_WORLD);

      // Procedimiento original
      cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
      MPI_Ssend( &aux, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

      cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
      MPI_Ssend( &aux, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

      cout <<"Filósofo " <<id <<" comienza a comer" <<endl ;
      sleep_for( milliseconds( aleatorio<10,100>() ) );

      cout <<"Filósofo " <<id <<" suelta ten. izq. " <<id_ten_izq <<endl;
      MPI_Ssend( &aux, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

      cout<< "Filósofo " <<id <<" suelta ten. der. " <<id_ten_der <<endl;
      MPI_Ssend( &aux, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

      // Tras soltar ambos tenedores el filósofo envía la petición de levantarse
      cout <<"Filósofo " << id << " solicita levantarse" <<endl;
      MPI_Ssend(&aux, 1, MPI_INT, id_camarero, etiq_liberar_asiento, MPI_COMM_WORLD);
      
      cout << "Filósofo " << id << " comienza a pensar" << endl;
      sleep_for( milliseconds( aleatorio<10,100>() ) );

    }
}
// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
  int valor, id_filosofo ;  // valor recibido, identificador del filósofo
  MPI_Status estado ;       // metadatos de las dos recepciones

  int aux;
  while ( true )
    {
      MPI_Recv(&aux,1,MPI_INT,MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &estado);
      id_filosofo = estado.MPI_SOURCE;
      cout <<"\t\tTen. " <<id <<" ha sido cogido por filo. " <<id_filosofo <<endl;

      MPI_Recv(&aux, 1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD, &estado);
     
      cout <<"\t\tTen. "<< id<< " ha sido liberado por filo. " <<id_filosofo <<endl ;
    }
}


void funcion_camarero(){
  int aux, etiq_aceptable; // Buffer auxiliar y etiqueta receptible
  int filosofos_sentados = 0;
  
  MPI_Status estado ;       // metadatos de las dos recepciones

  while ( true ){
    if (filosofos_sentados < num_filosofos-1) {
      etiq_aceptable = MPI_ANY_TAG;
    } else {
      etiq_aceptable = etiq_liberar_asiento;
    }

    MPI_Recv(&aux, 1, MPI_INT, MPI_ANY_SOURCE, etiq_aceptable, MPI_COMM_WORLD, &estado);

    if (estado.MPI_TAG == etiq_pedir_asiento) {
      filosofos_sentados++;
      cout <<"\tCamarero recibe petición de sentamiento del filósofo " << estado.MPI_SOURCE << ". Total sentados: " << filosofos_sentados << endl;
    } else if (estado.MPI_TAG == etiq_liberar_asiento) {
      filosofos_sentados--;
      cout <<"\tCamarero recibe petición levantamiento del filosofo " << estado.MPI_SOURCE << ". Total sentados: " << filosofos_sentados << endl;
    } 
  }
}
// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
  int id_propio, num_procesos_actual ;

  MPI_Init( &argc, &argv );
  MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
  MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


  if ( num_procesos == num_procesos_actual )
    {
      if(id_propio == 0)
	funcion_camarero();
      // ejecutar la función correspondiente a 'id_propio'
      else if ( id_propio % 2 == 0 )          // si es par
	funcion_filosofos( id_propio ); //   es un filósofo
      else                               // si es impar
	funcion_tenedores( id_propio ); //   es un tenedor
    }
  else
    {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
	{ cout << "el número de procesos esperados es:    " << num_procesos << endl
	       << "el número de procesos en ejecución es: " << num_procesos_actual << endl
	       << "(programa abortado)" << endl ;
	}
    }

  MPI_Finalize( );
  return 0;
}

// ---------------------------------------------------------------------
