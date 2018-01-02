// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: prodcons_multiples.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con múltiples productores y consumidores)
//
// -----------------------------------------------------------------------------

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
  num_prods             = 4 ,
  num_cons              = 5 ,
  id_max_productor      = num_prods -1 ,
  id_buffer             = num_prods ,
  id_max_consumidor     = num_prods + 1 + num_cons -1 ,
  num_procesos_esperado = 1+num_prods+num_cons ,
  num_items             = num_prods * num_cons *2,
  num_items_a_producir = num_items / num_prods,
  num_items_a_consumir = num_items /num_cons,
	// Debe ser múltiplo del número de prod y de consumidores 
  tam_vector            = 20,
  etiq_prod 		= 1,
  etiq_cons		= 2,
  etiq_buffer		= 3;

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
// producir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir(int id)
{
  static int contador = id* num_items_a_producir;
  sleep_for( milliseconds( aleatorio<10,100>()) );
  contador++ ;
  cout << "Productor " << id << " ha producido valor " << contador << endl << flush;
  return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor(int num_orden)
{
  for ( unsigned int i= 0 ; i < num_items_a_producir ; i++ )
    {
      // producir valor
      int valor_prod = producir(num_orden);
      // enviar valor
      cout << "Productor " << num_orden << "  va a enviar valor " << valor_prod << endl << flush;
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, etiq_prod, MPI_COMM_WORLD );
    }
}
// ---------------------------------------------------------------------

void consumir( int valor_cons, int id )
{
  // espera bloqueada
  sleep_for( milliseconds( aleatorio<110,200>()) );
  cout << "\tConsumidor " << id << " ha consumido valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor(int num_orden)
{
  int         peticion,
    valor_rec = 1 ;
  MPI_Status  estado ;

  for( unsigned int i=0 ; i < num_items_a_consumir; i++ )
    {
      MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD);
      MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, etiq_buffer, MPI_COMM_WORLD,&estado );
      cout << "\tConsumidor " << num_orden << "  ha recibido valor " << valor_rec << endl << flush ;
      consumir( valor_rec, num_orden );
    }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
  int        buffer[tam_vector],      // buffer con celdas ocupadas y vacías
    valor,                   // valor recibido o enviado
    primera_libre       = 0, // índice de primera celda libre
    primera_ocupada     = 0, // índice de primera celda ocupada
    num_celdas_ocupadas = 0, // número de celdas ocupadas
    etiq_emisor_aceptable ;    // etiqueta de emisor aceptable
  MPI_Status estado ;                 // metadatos del mensaje recibido

  for( unsigned int i=0 ; i < num_items*2 ; i++ )
    {
      // 1. determinar si puede enviar solo prod., solo cons, o todos

      if ( num_celdas_ocupadas == 0 )               // si buffer vacío
	etiq_emisor_aceptable = etiq_prod ;       // $~~~$ solo prod.
      else if ( num_celdas_ocupadas == tam_vector ) // si buffer lleno
	etiq_emisor_aceptable = etiq_cons ;      // $~~~$ solo cons.
      else                                          // si no vacío ni lleno
	etiq_emisor_aceptable = MPI_ANY_TAG ;     // $~~~$ cualquiera

      // 2. recibir un mensaje del emisor o emisores aceptables

      MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_emisor_aceptable, MPI_COMM_WORLD, &estado );

      // 3. procesar el mensaje recibido

      switch( estado.MPI_TAG ) // leer emisor del mensaje en metadatos
	{
	case etiq_prod: // si ha sido el productor: insertar en buffer
	  buffer[primera_libre] = valor ;
	  primera_libre = (primera_libre+1) % tam_vector ;
	  num_celdas_ocupadas++ ;
	  cout << "\t\tBuffer ha recibido valor " << valor << endl ;
	  break;

	case etiq_cons: // si ha sido el consumidor: extraer y enviarle
	  valor = buffer[primera_ocupada] ;
	  primera_ocupada = (primera_ocupada+1) % tam_vector ;
	  num_celdas_ocupadas-- ;
	  cout << "\t\tBuffer va a enviar valor " << valor << endl ;
	  MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_buffer, MPI_COMM_WORLD);
	  break;
	}
    }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  int id_propio, num_procesos_actual;

  // inicializar MPI, leer identif. de proceso y número de procesos
  MPI_Init( &argc, &argv );
  MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
  MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

  if ( num_procesos_esperado == num_procesos_actual )
    {
      // ejecutar la operación apropiada a 'id_propio'
      if ( id_propio <= id_max_productor ) // Si está entre 0 y el id max 
	funcion_productor(id_propio);
      else if ( id_propio == id_buffer ) // Si coincide con el id del buffer
	funcion_buffer();
      else				 // En caso contrario se usa
					 // el consumidor
	funcion_consumidor(id_propio);
    }
  else
    {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
	{ cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
	       << "el número de procesos en ejecución es: " << num_procesos_actual << endl
	       << "(programa abortado)" << endl ;
	}
    }

  // al terminar el proceso, finalizar MPI
  MPI_Finalize( );
  return 0;
}
