// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodcons_1.cpp
// Ejemplo de un monitor en C++11 con semántica SC, para el problema
// del productor/consumidor, con con múltiples productores y consumidores.
// Opcion FIFO
//
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>

using namespace std ;

const int n_productoras = 2;
const int n_consumidoras = 4;
bool DEBUG_MODE = false;
constexpr int
num_items  = 40 ;     // número de items a producir/consumir

mutex
mtx ;                 // mutex de escritura en pantalla
unsigned
cont_prod[num_items], // contadores de verificación: producidos
  cont_cons[num_items]; // contadores de verificación: consumidos

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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato()
{
  static int contador = 0 ;
  this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
  mtx.lock();
  cout << "producido: " << contador << endl << flush ;
  mtx.unlock();
  cont_prod[contador] ++ ;
  return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
  if ( num_items <= dato )
    {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
    }
  cont_cons[dato] ++ ;
  this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
  mtx.lock();
  cout << "                  consumido: " << dato << endl ;
  mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
  for( unsigned i = 0 ; i < num_items ; i++ )
    {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
    }
}

//----------------------------------------------------------------------

void test_contadores()
{
  bool ok = true ;
  cout << "comprobando contadores ...." << flush ;

  for( unsigned i = 0 ; i < num_items ; i++ )
    {
      if ( cont_prod[i] != 1 )
	{
	  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
	  ok = false ;
	}
      if ( cont_cons[i] != 1 )
	{
	  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
	  ok = false ;
	}
    }
  if (ok)
    cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, un prod. y un cons.

class ProdCons1SC
{
private:
  static const int           // constantes:
  num_celdas_total = 10;   //  núm. de entradas del buffer
  int                        // variables permanentes
  buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
    primera_libre ;          //  indice de celda de la próxima inserción
  
  // Variable pos_lectura
  int pos_lectura;
  int n_escrituras;
  
  mutex
  cerrojo_monitor ;        // cerrojo del monitor
  condition_variable         // colas condicion:
  ocupadas,                //  cola donde espera el consumidor (n>0)
    libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

public:                    // constructor y métodos públicos
  ProdCons1SC(  ) ;           // constructor
  int  leer();                // extraer un valor (sentencia L) (consumidor)
  void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdCons1SC::ProdCons1SC(  )
{
  primera_libre = 0 ;
  pos_lectura = 0;
  n_escrituras = 0;
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdCons1SC::leer(  )
{
  // ganar la exclusión mutua del monitor con una guarda
  unique_lock<mutex> guarda( cerrojo_monitor );

  // esperar bloqueado hasta que 0 < num_celdas_ocupadas
  if(DEBUG_MODE)
    cout << "Intento leer: Primera libre = " << primera_libre << ", pos_lectura = " << pos_lectura << endl;
  if ( primera_libre == pos_lectura )
    ocupadas.wait( guarda );

  if(DEBUG_MODE)
    cout << "Se ha cumplido la condición de espera de la hebra consumidora" << endl;
  // hacer la operación de lectura, actualizando estado del monitor
  assert( primera_libre != pos_lectura  );
  const int valor = buffer[pos_lectura] ;
  pos_lectura = (pos_lectura + 1) % num_celdas_total  ;
  n_escrituras--;

  if(DEBUG_MODE)
    cout << "Se consume buffer[" << pos_lectura << "] = " << buffer[pos_lectura] << endl;


  // señalar al productor que hay un hueco libre, por si está esperando
  libres.notify_one();

  // devolver valor
  return valor ;
}
// -----------------------------------------------------------------------------

void ProdCons1SC::escribir( int valor )
{
  // ganar la exclusión mutua del monitor con una guarda
  unique_lock<mutex> guarda( cerrojo_monitor );

  // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total

  if(DEBUG_MODE)
    cout << "Intento escribir el " << valor << ": Primera libre = " << primera_libre << ", pos_lectura = " << pos_lectura << " \nNúmero escrituras: " << n_escrituras << endl;
  if ( primera_libre == num_celdas_total  && n_escrituras == 9)
    libres.wait( guarda );

  //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
  assert( !(primera_libre == num_celdas_total  && n_escrituras == 9) );

  // hacer la operación de inserción, actualizando estado del monitor
  buffer[primera_libre] = valor ;
  if(DEBUG_MODE)
    cout << "buffer[" << primera_libre << "] = " << buffer[primera_libre] << endl;
  primera_libre = (primera_libre + 1) % num_celdas_total ;
  n_escrituras++;
  
  // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
  ocupadas.notify_one();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( ProdCons1SC * monitor, int id )
{
  for( unsigned i = id ; i < num_items ; i+=n_productoras )
    {
      int valor = producir_dato() ;
      monitor->escribir( valor );
    }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( ProdCons1SC * monitor, int id )
{
  for( unsigned i = id ; i < num_items ; i+=n_consumidoras )
    {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
    }
}
// -----------------------------------------------------------------------------

int main()
{
  cout << "-------------------------------------------------------------------------------" << endl
       << "Problema de los productores-consumidores (multiples prod/cons, Monitor SC, buffer FIFO). " << endl
       << "-------------------------------------------------------------------------------" << endl
       << flush ;

  ProdCons1SC monitor ;

   thread hebra_productora0 ( funcion_hebra_productora, &monitor,0 ),
     hebra_productora1 ( funcion_hebra_productora, &monitor,1 ),
     hebra_consumidora0( funcion_hebra_consumidora, &monitor,0 ),
     hebra_consumidora1( funcion_hebra_consumidora, &monitor,1 ),
     hebra_consumidora2( funcion_hebra_consumidora, &monitor,2 ),
     hebra_consumidora3( funcion_hebra_consumidora, &monitor,3 );
   

   hebra_productora0.join() ;
   hebra_productora1.join() ;
   
   hebra_consumidora0.join() ;
   hebra_consumidora1.join() ;
   hebra_consumidora2.join() ;
   hebra_consumidora3.join() ;

  // comprobar que cada item se ha producido y consumido exactamente una vez
  test_contadores() ;
}
