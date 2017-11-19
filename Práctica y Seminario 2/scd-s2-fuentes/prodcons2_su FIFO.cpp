// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodcons_1.cpp
// Ejemplo de un monitor en C++11 con semántica SC, para el problema
// del productor/consumidor, con múltiples productores y consumidores.
// Opcion LIFO (stack)
//
// Historial:
// Creado en Julio de 2017
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.hpp"
using namespace HM;
using namespace std ;

constexpr int
   num_items  = 40 ;     // número de items a producir/consumir

const int n_productoras = 2;
const int n_consumidoras = 4;

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
// clase para monitor buffer, version FIFO, semántica SU, varios prodcons.

class ProdCons1SU:public HoareMonitor
{
 private:
 static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   primera_libre ;          //  indice de celda de la próxima inserción

  int primera_ocupada;
  bool todas_ocupadas, todas_libres;
  //mutex
  //cerrojo_monitor ;        // cerrojo del monitor
 CondVar         // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdCons1SU(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdCons1SU::ProdCons1SU(  )
{
  todas_ocupadas = false;
  todas_libres = true;
   primera_libre = 0 ;
   primera_ocupada = 0;
   ocupadas = newCondVar();
   libres = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdCons1SU::leer(  )
{
   
   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if ( todas_libres )
      ocupadas.wait(  );

   // hacer la operación de lectura, actualizando estado del monitor
   const int valor = buffer[primera_ocupada] ;
   primera_ocupada = (primera_ocupada + 1) % num_celdas_total  ;
   if(primera_ocupada == primera_libre)
     todas_libres = true;
   todas_ocupadas = false;


   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdCons1SU::escribir( int valor )
{
   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( todas_ocupadas)
      libres.wait(  );

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
  primera_libre = (primera_libre + 1) % num_celdas_total ;
  if(primera_ocupada == primera_libre)
    todas_ocupadas = true;
  todas_libres = false;


   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdCons1SU> monitor, int id )
{
   for( unsigned i = id ; i < num_items ; i+=n_productoras)
   {
      int valor = producir_dato() ;
      //cout << "soy hebra productora " << id << " produzco: " << valor<< endl;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdCons1SU> monitor, int id )
{
   for( unsigned i = id ; i < num_items ; i+=n_consumidoras )
   {
      int valor = monitor->leer();
      //cout << "Soy hebra consumidora " << id << " consumo: " << valor<< endl;
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (multiples prod/cons, Monitor SU, buffer LIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdCons1SU> monitor = Create<ProdCons1SU>() ;

   thread hebras_prod[n_productoras];
   thread hebras_cons[n_consumidoras];

   for(int i = 0; i < n_productoras;i++)
     hebras_prod[i] = thread (funcion_hebra_productora, monitor, i);

   for(int i = 0; i < n_consumidoras;i++)
     hebras_cons[i] = thread (funcion_hebra_consumidora,monitor,i);
   

   for(int i = 0; i < n_productoras;i++)
     hebras_prod[i].join();

   for(int i = 0; i < n_consumidoras;i++)
     hebras_cons[i].join();
       
   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
