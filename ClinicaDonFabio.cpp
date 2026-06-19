/*
   Trabajo Final (Estructura de Datos y Algoritmos)
   Equipo Azul: "Turnos en la clinica de Don Fabio"

   Sistema de gestion de turnos para la consulta de Don Fabio. Permite:
     a) Registrar pacientes (nombre, edad, nivel de prioridad).
     b) Cola de atencion priorizada (primero el mas urgente).
     c) Historial de pacientes atendidos en una lista enlazada.
     d) Pila de citas canceladas para poder reactivarlas.
     e) Busqueda lineal de un paciente por su nombre.
     f) Ordenar los atendidos por edad o por fecha de atencion.
     g) Conteo recursivo de atendidos mayores de 60 anios.

   Estructuras usadas: cola priorizada, lista enlazada y pila (con punteros)
   y un arreglo dinamico para el ordenamiento del historial.
*/

#include <iostream>
#include <string>
#include <ctime>
#include <cctype>
#include <cmath>
#include <limits>
using namespace std;

const string LINEA  = string(50, '-');
const string DLINEA = string(50, '=');

// Etiquetas de prioridad (arreglo estatico). 1=Urgente, 2=Por turno
const string ETIQUETA_PRIORIDAD[3] = {"", "Urgente", "Por turno"};

// =================================================================
//  FUNCIONES AUXILIARES
// =================================================================

string aMayusculas(string texto)
{
    for (int i = 0; i < (int)texto.length(); i++)
        texto[i] = toupper((unsigned char)texto[i]);
    return texto;
}

// True si 'nombre' contiene a 'sub', sin distinguir mayusculas. Asi la
// busqueda parcial: "ana" encuentra a "Ana Diaz".
bool contiene(string nombre, string sub)
{
    return aMayusculas(nombre).find(aMayusculas(sub)) != string::npos;
}

string recortar(string texto)
{
    int inicio = 0;
    int fin = (int)texto.length() - 1;

    while (inicio <= fin && isspace((unsigned char)texto[inicio]))
        inicio++;
    while (fin >= inicio && isspace((unsigned char)texto[fin]))
        fin--;

    return texto.substr(inicio, fin - inicio + 1);
}

// Fecha y hora actual ajustada a Republica Dominicana (UTC-4). Se usa
// gmtime (hora UTC) y se le restan 4 horas, asi la hora sale correcta
// aunque el servidor (por ejemplo OnlineGDB) este en otra zona horaria.
string fechaHoraActual()
{
    time_t ahora = time(0);
    ahora = ahora - 4 * 3600;
    tm* t = gmtime(&ahora);

    char buffer[20];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M", t);

    return string(buffer);
}

// Lee un numero entero validando que el usuario no escriba letras.
// Si el flujo se cierra (EOF) devuelve -1 para salir de forma ordenada.
int leerEntero(string mensaje)
{
    int valor;
    while (true)
    {
        cout << mensaje;
        cin >> valor;

        if (cin.fail())
        {
            if (cin.eof())
                return -1;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << " Entrada invalida. Escriba un numero." << endl;
        }
        else
        {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return valor;
        }
    }
}

// Lee una linea de texto (permite nombres con espacios), recorta los
// espacios sobrantes y no acepta campos vacios. Si el flujo se cierra
// (EOF) devuelve "0", que el programa interpreta como cancelar.
string leerTexto(string mensaje)
{
    string texto;
    cout << mensaje;
    while (true)
    {
        if (!getline(cin, texto))
            return "0";
        texto = recortar(texto);
        if (!texto.empty())
            return texto;
        cout << " El campo no puede quedar vacio. Intente de nuevo." << endl;
        cout << mensaje;
    }
}

// Lee una respuesta de si/no. Acepta "s", "si", "n", "no" sin distinguir
// mayusculas; si el flujo se cierra (EOF) se asume que no.
bool confirmar(string mensaje)
{
    string r;
    while (true)
    {
        cout << mensaje;
        if (!(cin >> r))
            return false;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        r = aMayusculas(recortar(r));
        if (r == "S" || r == "SI") return true;
        if (r == "N" || r == "NO") return false;
        cout << " Responda con 's' o 'n'." << endl;
    }
}

// =================================================================
//  CLASE PACIENTE
//  Sirve como nodo para la cola, la lista y la pila. El id es un
//  numero fijo que se asigna al registrar y nunca se reutiliza, para
//  llevar la trazabilidad de cada paciente.
// =================================================================
class Paciente
{
public:
    int    id;            // identificador fijo / numero de turno
    string nombre;
    int    edad;
    int    prioridad;      // 1 Urgente, 2 Por turno
    int    ordenAtencion; // orden cronologico en que fue atendido
    string fechaAtencion;
    Paciente* sig;

    Paciente(int identificador, string nom, int ed, int urg)
    {
        id = identificador;
        nombre = nom;
        edad = ed;
        prioridad = urg;
        ordenAtencion = 0;
        fechaAtencion = "";
        sig = nullptr;
    }
};

// =================================================================
//  IMPRESION DE TABLAS (formato compartido por todas las vistas)
//  Calcula el ancho de la columna del nombre e imprime encabezado,
//  filas y cierre. Columnas alineadas con cout.width()/left; con
//  conFecha se agrega la columna "Atendido".
// =================================================================

// Recorta un texto a un ancho dado, terminando en "..." si se pasa. Sirve
// como tope de seguridad para un nombre desmedido; en uso normal no recorta.
string ajustarAncho(string texto, int ancho)
{
    if ((int)texto.length() <= ancho) return texto;
    return texto.substr(0, ancho - 3) + "...";
}

// Ajusta el ancho de la columna del nombre al nombre mas largo (con un minimo
// y un tope) para que se vean completos sin descuadrar la tabla.
int anchoNombre(Paciente* inicio)
{
    int maximo = 12;
    for (Paciente* p = inicio; p != nullptr; p = p->sig)
        if ((int)p->nombre.length() > maximo) maximo = (int)p->nombre.length();
    if (maximo > 40) maximo = 40;
    return maximo + 2;
}

int anchoNombre(Paciente** arr, int n)
{
    int maximo = 12;
    for (int i = 0; i < n; i++)
        if ((int)arr[i]->nombre.length() > maximo) maximo = (int)arr[i]->nombre.length();
    if (maximo > 40) maximo = 40;
    return maximo + 2;
}

void imprimirEncabezado(bool conFecha, int anchoNom)
{
    int largo = 7 + anchoNom + 7 + 11 + (conFecha ? 16 : 0);
    string linea = string(largo, '-');
    cout << " " << linea << endl;
    cout << " " << left;
    cout.width(7);        cout << "Turno";
    cout.width(anchoNom); cout << "Nombre";
    cout.width(7);        cout << "Edad";
    cout.width(11);       cout << "Prioridad";
    if (conFecha) cout << "Atendido";
    cout << endl;
    cout << " " << linea << endl;
}

void imprimirFila(Paciente* p, bool conFecha, int anchoNom)
{
    cout << " " << left;
    cout.width(7);        cout << ("#" + to_string(p->id));
    cout.width(anchoNom); cout << ajustarAncho(p->nombre, anchoNom - 2);
    cout.width(7);        cout << p->edad;
    cout.width(11);       cout << ETIQUETA_PRIORIDAD[p->prioridad];
    if (conFecha) cout << p->fechaAtencion;
    cout << endl;
}

void imprimirCierre(bool conFecha, string etiqueta, int total, int anchoNom)
{
    int largo = 7 + anchoNom + 7 + 11 + (conFecha ? 16 : 0);
    string linea = string(largo, '-');
    cout << " " << linea << endl;
    cout << " " << etiqueta << ": " << total << endl;
}

// =================================================================
//  COLA DE ATENCION PRIORIZADA
//  Lista enlazada ordenada por prioridad: los Urgentes van primero y
//  el resto por orden de llegada (turno). Dentro de cada grupo se
//  respeta el orden en que llegaron.
// =================================================================
class ColaPrioridad
{
private:
    Paciente* frente;
    int total;

public:
    ColaPrioridad() { frente = nullptr; total = 0; }

    bool vacia()          { return frente == nullptr; }
    int  cantidad()       { return total; }
    Paciente* getInicio() { return frente; }
    Paciente* siguienteAAtender() { return frente; } // el siguiente, sin sacarlo

    // Inserta segun la prioridad: los Urgentes (1) quedan delante de los de
    // turno (2). Con igual prioridad, se respeta el orden de llegada.
    void encolar(int id, string nombre, int edad, int prioridad)
    {
        Paciente* nuevo = new Paciente(id, nombre, edad, prioridad);

        if (frente == nullptr || prioridad < frente->prioridad)
        {
            nuevo->sig = frente;
            frente = nuevo;
        }
        else
        {
            Paciente* actual = frente;
            while (actual->sig != nullptr && actual->sig->prioridad <= prioridad)
                actual = actual->sig;
            nuevo->sig = actual->sig;
            actual->sig = nuevo;
        }
        total++;
    }

    Paciente* atender()
    {
        if (vacia()) return nullptr;
        Paciente* p = frente;
        frente = frente->sig;
        p->sig = nullptr;
        total--;
        return p;
    }

    // Busqueda lineal por nombre (sin distinguir mayusculas).
    Paciente* buscar(string nombre)
    {
        string objetivo = aMayusculas(nombre);
        for (Paciente* p = frente; p != nullptr; p = p->sig)
            if (aMayusculas(p->nombre) == objetivo)
                return p;
        return nullptr;
    }

    // Quita de la cola al paciente con ese id (turno) y lo devuelve,
    // reenlazando los vecinos. nullptr si no esta.
    Paciente* removerPorId(int id)
    {
        Paciente* actual = frente;
        Paciente* anterior = nullptr;

        while (actual != nullptr && actual->id != id)
        {
            anterior = actual;
            actual = actual->sig;
        }

        if (actual == nullptr)
            return nullptr;

        if (anterior == nullptr)
            frente = actual->sig;
        else
            anterior->sig = actual->sig;

        actual->sig = nullptr;
        total--;
        return actual;
    }

    void mostrar()
    {
        if (vacia())
        {
            cout << " No hay pacientes en espera." << endl;
            return;
        }
        int an = anchoNombre(frente);
        imprimirEncabezado(false, an);
        for (Paciente* p = frente; p != nullptr; p = p->sig)
            imprimirFila(p, false, an);
        imprimirCierre(false, "Pacientes en espera", total, an);
    }

    ~ColaPrioridad()
    {
        while (frente != nullptr)
        {
            Paciente* aux = frente;
            frente = frente->sig;
            delete aux;
        }
    }
};

// =================================================================
//  HISTORIAL DE PACIENTES ATENDIDOS
//  Lista enlazada simple. Conserva el orden en que se atendio a
//  cada paciente.
// =================================================================
class HistorialPacientes
{
private:
    Paciente* cabeza;
    int total;

    // Recorrido recursivo: cuenta los atendidos mayores de 60 anios.
    int contarMayores60Rec(Paciente* nodo)
    {
        if (nodo == nullptr) return 0;
        int suma = (nodo->edad > 60) ? 1 : 0;
        return suma + contarMayores60Rec(nodo->sig);
    }

public:
    HistorialPacientes() { cabeza = nullptr; total = 0; }

    int cantidad()         { return total; }
    Paciente* getCabeza()  { return cabeza; }

    // Agrega al final para conservar el orden de atencion.
    void agregar(Paciente* p)
    {
        if (cabeza == nullptr)
            cabeza = p;
        else
        {
            Paciente* actual = cabeza;
            while (actual->sig != nullptr)
                actual = actual->sig;
            actual->sig = p;
        }
        total++;
    }

    int contarMayores60() { return contarMayores60Rec(cabeza); }

    void mostrar()
    {
        if (cabeza == nullptr)
        {
            cout << " Aun no hay pacientes atendidos." << endl;
            return;
        }
        int an = anchoNombre(cabeza);
        imprimirEncabezado(true, an);
        Paciente* actual = cabeza;
        while (actual != nullptr)
        {
            imprimirFila(actual, true, an);
            actual = actual->sig;
        }
        imprimirCierre(true, "Total de pacientes atendidos", total, an);
    }

    // Copia los punteros a un arreglo dinamico para ordenarlos sin tocar la lista.
    Paciente** copiarAArreglo(int& n)
    {
        n = total;
        if (n == 0) return nullptr;

        Paciente** arr = new Paciente*[n];
        Paciente* actual = cabeza;
        int i = 0;
        while (actual != nullptr)
        {
            arr[i++] = actual;
            actual = actual->sig;
        }
        return arr;
    }

    ~HistorialPacientes()
    {
        while (cabeza != nullptr)
        {
            Paciente* aux = cabeza;
            cabeza = cabeza->sig;
            delete aux;
        }
    }
};

// =================================================================
//  PILA DE CITAS CANCELADAS (LIFO)
//  Guarda las citas canceladas para poder reactivarlas. Se puede
//  reactivar a un paciente concreto aunque no este en el tope.
// =================================================================
class PilaCancelados
{
private:
    Paciente* tope;
    int total;

public:
    PilaCancelados() { tope = nullptr; total = 0; }

    bool vacia()    { return tope == nullptr; }
    int  cantidad() { return total; }
    Paciente* getTope() { return tope; }

    void apilar(Paciente* p)
    {
        p->sig = tope;
        tope = p;
        total++;
    }

    Paciente* buscar(string nombre)
    {
        string objetivo = aMayusculas(nombre);
        Paciente* actual = tope;
        while (actual != nullptr)
        {
            if (aMayusculas(actual->nombre) == objetivo)
                return actual;
            actual = actual->sig;
        }
        return nullptr;
    }

    // Saca de la pila al paciente con ese id (turno), sin importar en que
    // posicion este: aparta las citas de encima en una pila auxiliar,
    // extrae la buscada y devuelve el resto a su lugar. nullptr si no esta.
    Paciente* removerPorId(int id)
    {
        Paciente* auxiliar = nullptr;
        Paciente* encontrado = nullptr;

        // Sacamos del tope y apartamos hasta dar con el paciente buscado
        while (tope != nullptr && encontrado == nullptr)
        {
            Paciente* temp = tope;
            tope = tope->sig;

            if (temp->id == id)
            {
                temp->sig = nullptr;
                encontrado = temp;
                total--;
            }
            else
            {
                temp->sig = auxiliar;
                auxiliar = temp;
            }
        }

        // Devolvemos las apartadas a la pila en su orden original
        while (auxiliar != nullptr)
        {
            Paciente* temp = auxiliar;
            auxiliar = auxiliar->sig;
            temp->sig = tope;
            tope = temp;
        }

        return encontrado;
    }

    void mostrar()
    {
        if (vacia())
        {
            cout << " No hay citas canceladas." << endl;
            return;
        }
        int an = anchoNombre(tope);
        imprimirEncabezado(false, an);
        Paciente* actual = tope;
        while (actual != nullptr)
        {
            imprimirFila(actual, false, an);
            actual = actual->sig;
        }
        imprimirCierre(false, "Citas canceladas", total, an);
    }

    ~PilaCancelados()
    {
        while (tope != nullptr)
        {
            Paciente* aux = tope;
            tope = tope->sig;
            delete aux;
        }
    }
};

// =================================================================
//  CLASE CLINICA
//  Coordina las tres estructuras y concentra la logica del sistema.
// =================================================================
class Clinica
{
private:
    ColaPrioridad      cola;
    HistorialPacientes historial;
    PilaCancelados     cancelados;
    int siguienteId;        // ID fijo que se asigna al registrar
    int contadorAtenciones; // orden cronologico de atencion

    // Ordenamiento por insercion sobre el arreglo de punteros.
    // criterio 1 = por edad; criterio 2 = por fecha (orden de atencion).
    void insertionSort(Paciente** arr, int n, int criterio)
    {
        for (int i = 1; i < n; i++)
        {
            Paciente* clave = arr[i];
            int j = i - 1;
            while (j >= 0 && esMayor(arr[j], clave, criterio))
            {
                arr[j + 1] = arr[j];
                j--;
            }
            arr[j + 1] = clave;
        }
    }

    bool esMayor(Paciente* a, Paciente* b, int criterio)
    {
        if (criterio == 1) return a->edad > b->edad;
        return a->ordenAtencion > b->ordenAtencion;
    }

    // Indica si ya hay un paciente activo (en espera o con cita cancelada)
    // con ese nombre. No revisa el historial, porque un paciente que regresa
    // otro dia comparte nombre con su registro anterior y avisaria de mas.
    bool nombreActivoExiste(string nombre)
    {
        return cola.buscar(nombre) != nullptr ||
               cancelados.buscar(nombre) != nullptr;
    }

    // Junta las coincidencias parciales por nombre recorriendo desde 'inicio'.
    // Si hay varias, las muestra numeradas para que el usuario elija. Devuelve
    // el id (turno) del paciente elegido, o -1 si no hay nadie o se cancela.
    int elegirPorNombre(Paciente* inicio, string texto)
    {
        int total = 0;
        for (Paciente* p = inicio; p != nullptr; p = p->sig)
            if (contiene(p->nombre, texto)) total++;

        if (total == 0)
        {
            cout << "\n No se encontro a nadie con ese nombre." << endl;
            return -1;
        }

        Paciente** coincide = new Paciente*[total];
        int n = 0;
        for (Paciente* p = inicio; p != nullptr; p = p->sig)
            if (contiene(p->nombre, texto)) coincide[n++] = p;

        int elegido = 0;   // por defecto, la unica coincidencia

        if (n > 1)
        {
            int an = anchoNombre(coincide, n);
            string sep = string(16 + an, '-');
            cout << "\n Hay " << n << " coincidencias:" << endl;
            cout << " " << sep << endl;
            cout << " " << left;
            cout.width(5);  cout << "No.";
            cout.width(7);  cout << "Turno";
            cout.width(an); cout << "Nombre";
            cout << "Edad" << endl;
            cout << " " << sep << endl;
            for (int i = 0; i < n; i++)
            {
                cout << " " << left;
                cout.width(5);  cout << (to_string(i + 1) + ".");
                cout.width(7);  cout << ("#" + to_string(coincide[i]->id));
                cout.width(an); cout << ajustarAncho(coincide[i]->nombre, an - 2);
                cout << coincide[i]->edad << endl;
            }
            cout << " " << sep << endl;

            int opcion;
            do
            {
                opcion = leerEntero(" Cual? (numero, 0 para volver): ");
                if (opcion == -1 || opcion == 0)
                {
                    cout << "\n Operacion cancelada." << endl;
                    delete[] coincide;
                    return -1;
                }
                if (opcion < 1 || opcion > n)
                    cout << " Numero fuera de rango." << endl;
            } while (opcion < 1 || opcion > n);
            elegido = opcion - 1;
        }

        int id = coincide[elegido]->id;
        delete[] coincide;
        return id;
    }

public:
    Clinica()
    {
        siguienteId = 1;
        contadorAtenciones = 0;
    }

    int pacientesEnEspera() { return cola.cantidad(); }

    void registrarPaciente()
    {
        cout << "\n >> REGISTRAR PACIENTE" << endl;
        cout << LINEA << endl;

        string nombre = leerTexto(" Nombre del paciente (0 para cancelar): ");
        if (nombre == "0")
        {
            cout << " Registro cancelado. Volviendo al menu." << endl;
            return;
        }

        // Si ya hay un paciente activo con ese nombre, se avisa para poder
        // diferenciarlos. El aviso informa, pero no obliga: pueden ser dos
        // personas distintas.
        while (nombreActivoExiste(nombre))
        {
            cout << "\n Aviso: ya hay un paciente activo llamado \"" << nombre << "\"." << endl;
            cout << " Conviene diferenciarlos con un segundo nombre o apellido." << endl;
            int decision = leerEntero(" [1] Reescribir nombre  [2] Registrar de todas formas  [0] Cancelar: ");

            if (decision == -1 || decision == 0)
            {
                cout << " Registro cancelado. Volviendo al menu." << endl;
                return;
            }
            if (decision == 2)
                break;

            if (decision == 1)
            {
                nombre = leerTexto(" Nuevo nombre (0 para cancelar): ");
                if (nombre == "0")
                {
                    cout << " Registro cancelado. Volviendo al menu." << endl;
                    return;
                }
            }
        }

        int edad;
        do
        {
            edad = leerEntero(" Edad: ");
            if (edad == -1) { cout << " Registro cancelado." << endl; return; }
            if (edad <= 0 || edad > 120)
                cout << " Ingrese una edad valida (1 a 120)." << endl;
        } while (edad <= 0 || edad > 120);

        int prioridad;
        do
        {
            prioridad = leerEntero(" Nivel de prioridad (1=Urgente, 2=Por turno): ");
            if (prioridad == -1) { cout << " Registro cancelado." << endl; return; }
            if (prioridad < 1 || prioridad > 2)
                cout << " Opcion invalida. Use 1 o 2." << endl;
        } while (prioridad < 1 || prioridad > 2);

        cola.encolar(siguienteId, nombre, edad, prioridad);
        cout << "\n Paciente registrado con el turno #" << siguienteId
             << " (Prioridad: " << ETIQUETA_PRIORIDAD[prioridad] << ")." << endl;
        siguienteId++;
    }

    void atenderPaciente()
    {
        cout << "\n >> ATENDER SIGUIENTE PACIENTE" << endl;

        if (cola.vacia())
        {
            cout << LINEA << endl;
            cout << " No hay pacientes en espera." << endl;
            return;
        }

        cola.mostrar();

        Paciente* siguiente = cola.siguienteAAtender();
        cout << "\n El siguiente en ser atendido es: #" << siguiente->id
             << " " << siguiente->nombre
             << " (Prioridad: " << ETIQUETA_PRIORIDAD[siguiente->prioridad] << ")." << endl;

        if (!confirmar(" Confirmar atencion? (s/n): "))
        {
            cout << "\n Atencion cancelada. El paciente sigue en la cola." << endl;
            return;
        }

        Paciente* p = cola.atender();
        contadorAtenciones++;
        p->ordenAtencion = contadorAtenciones;
        p->fechaAtencion = fechaHoraActual();
        historial.agregar(p);

        cout << "\n Atendiendo a " << p->nombre
             << " (Prioridad: " << ETIQUETA_PRIORIDAD[p->prioridad] << ")." << endl;
        cout << " Registrado en el historial el " << p->fechaAtencion << "." << endl;
    }

    void cancelarCita()
    {
        cout << "\n >> CANCELAR CITA" << endl;
        cout << LINEA << endl;

        if (cola.vacia())
        {
            cout << " No hay pacientes en espera." << endl;
            return;
        }

        cout << " Pacientes en espera:" << endl;
        cola.mostrar();

        string nombre = leerTexto("\n Nombre del paciente que cancela (0 para volver): ");
        if (nombre == "0")
        {
            cout << "\n Operacion cancelada." << endl;
            return;
        }

        int id = elegirPorNombre(cola.getInicio(), nombre);
        if (id == -1) return;

        Paciente* p = cola.removerPorId(id);
        cancelados.apilar(p);
        cout << "\n Cita de " << p->nombre
             << " cancelada y guardada por si desea reactivarla." << endl;
    }

    void reactivarCita()
    {
        cout << "\n >> REACTIVAR UNA CITA CANCELADA" << endl;
        cout << LINEA << endl;

        if (cancelados.vacia())
        {
            cout << " No hay citas canceladas para reactivar." << endl;
            return;
        }

        cout << " Citas canceladas:" << endl;
        cancelados.mostrar();

        string nombre = leerTexto("\n Nombre del paciente que desea reactivar (0 para volver): ");
        if (nombre == "0")
        {
            cout << "\n Operacion cancelada." << endl;
            return;
        }

        int id = elegirPorNombre(cancelados.getTope(), nombre);
        if (id == -1) return;

        Paciente* p = cancelados.removerPorId(id);
        cola.encolar(p->id, p->nombre, p->edad, p->prioridad);
        cout << "\n Cita de " << p->nombre
             << " reactivada y enviada de nuevo a la cola." << endl;
        delete p;
    }

    void buscarPaciente()
    {
        cout << "\n >> BUSCAR PACIENTE" << endl;
        cout << LINEA << endl;

        string texto = leerTexto(" Nombre o parte del nombre (0 para volver): ");
        if (texto == "0") { cout << " Volviendo al menu." << endl; return; }

        // Cota superior de coincidencias = total de pacientes en el sistema
        int maximo = cola.cantidad() + historial.cantidad() + cancelados.cantidad();
        if (maximo == 0)
        {
            cout << "\n No hay pacientes en el sistema." << endl;
            return;
        }

        // Arreglos dinamicos para juntar las coincidencias y su estado
        Paciente** encontrados = new Paciente*[maximo];
        string*    estados     = new string[maximo];
        int m = 0;

        // Recorremos las tres estructuras buscando coincidencias parciales
        for (Paciente* p = cola.getInicio(); p != nullptr; p = p->sig)
            if (contiene(p->nombre, texto)) { encontrados[m] = p; estados[m] = "En espera"; m++; }

        for (Paciente* p = historial.getCabeza(); p != nullptr; p = p->sig)
            if (contiene(p->nombre, texto)) { encontrados[m] = p; estados[m] = "Atendido"; m++; }

        for (Paciente* p = cancelados.getTope(); p != nullptr; p = p->sig)
            if (contiene(p->nombre, texto)) { encontrados[m] = p; estados[m] = "Cancelada"; m++; }

        if (m == 0)
        {
            cout << "\n No se encontro ningun paciente que coincida con \"" << texto << "\"." << endl;
            delete[] encontrados;
            delete[] estados;
            return;
        }

        int elegido = 0;   // por defecto, el unico encontrado

        if (m > 1)
        {
            int an = anchoNombre(encontrados, m);
            string sep = string(28 + an, '-');
            cout << "\n Se encontraron " << m << " coincidencias:" << endl;
            cout << " " << sep << endl;
            cout << " " << left;
            cout.width(5);  cout << "No.";
            cout.width(7);  cout << "Turno";
            cout.width(an); cout << "Nombre";
            cout.width(7);  cout << "Edad";
            cout << "Estado" << endl;
            cout << " " << sep << endl;
            for (int i = 0; i < m; i++)
            {
                cout << " " << left;
                cout.width(5);  cout << (to_string(i + 1) + ".");
                cout.width(7);  cout << ("#" + to_string(encontrados[i]->id));
                cout.width(an); cout << ajustarAncho(encontrados[i]->nombre, an - 2);
                cout.width(7);  cout << encontrados[i]->edad;
                cout << estados[i] << endl;
            }
            cout << " " << sep << endl;

            int opcion;
            do
            {
                opcion = leerEntero(" Cual desea ver? (numero, 0 para volver): ");
                if (opcion == -1 || opcion == 0)
                {
                    delete[] encontrados;
                    delete[] estados;
                    return;
                }
                if (opcion < 1 || opcion > m)
                    cout << " Numero fuera de rango." << endl;
            } while (opcion < 1 || opcion > m);

            elegido = opcion - 1;
        }

        Paciente* p = encontrados[elegido];
        cout << "\n Detalle del paciente:" << endl;
        cout << "   Turno    : #" << p->id << endl;
        cout << "   Nombre   : " << p->nombre << endl;
        cout << "   Edad     : " << p->edad << " anios" << endl;
        cout << "   Prioridad : " << ETIQUETA_PRIORIDAD[p->prioridad] << endl;
        cout << "   Estado   : " << estados[elegido] << endl;
        if (estados[elegido] == "Atendido")
            cout << "   Atendido : " << p->fechaAtencion << endl;

        delete[] encontrados;
        delete[] estados;
    }

    void ordenarHistorial()
    {
        cout << "\n >> ORDENAR HISTORIAL" << endl;
        cout << LINEA << endl;

        int n;
        Paciente** arr = historial.copiarAArreglo(n);
        if (n == 0)
        {
            cout << " El historial esta vacio." << endl;
            return;
        }

        int criterio;
        do
        {
            criterio = leerEntero(" Ordenar por (1=Edad, 2=Fecha de atencion): ");
            if (criterio == -1) { delete[] arr; return; }
            if (criterio != 1 && criterio != 2)
                cout << " Opcion invalida. Use 1 o 2." << endl;
        } while (criterio != 1 && criterio != 2);

        insertionSort(arr, n, criterio);

        cout << "\n Historial ordenado:" << endl;
        int an = anchoNombre(arr, n);
        imprimirEncabezado(true, an);
        for (int i = 0; i < n; i++)
            imprimirFila(arr[i], true, an);
        imprimirCierre(true, "Total de pacientes atendidos", n, an);

        delete[] arr;   // se libera el arreglo, no los pacientes
    }

    void mostrarColaActual()
    {
        cout << "\n >> SALA DE ESPERA (por prioridad)" << endl;
        cola.mostrar();
    }

    void mostrarHistorial()
    {
        cout << "\n >> HISTORIAL DE ATENDIDOS" << endl;
        historial.mostrar();
    }

    void mostrarCancelados()
    {
        cout << "\n >> CITAS CANCELADAS" << endl;
        cancelados.mostrar();
    }

    void mostrarEstadisticas()
    {
        cout << "\n >> ESTADISTICAS" << endl;
        cout << LINEA << endl;

        int atendidos = historial.cantidad();
        cout << " Pacientes en espera      : " << cola.cantidad() << endl;
        cout << " Pacientes atendidos      : " << atendidos << endl;
        cout << " Citas canceladas         : " << cancelados.cantidad() << endl;
        cout << " Mayores de 60 (recursivo): " << historial.contarMayores60() << endl;

        if (atendidos > 0)
        {
            int suma = 0;
            Paciente* actual = historial.getCabeza();
            while (actual != nullptr)
            {
                suma += actual->edad;
                actual = actual->sig;
            }
            double promedio = (double)suma / atendidos;
            cout << " Edad promedio (redondeada): " << round(promedio) << " anios" << endl;
        }
    }

    // Carga unos pacientes de ejemplo para poder probar de una vez.
    void cargarDatosPrueba()
    {
        cola.encolar(siguienteId++, "Juan Gomez Perez",            45, 1);  // Urgente
        cola.encolar(siguienteId++, "Rosa Mena Castillo",          38, 1);  // Urgente
        cola.encolar(siguienteId++, "Maria Perez Santana",         72, 2);  // Por turno
        cola.encolar(siguienteId++, "Pedro Luna Reyes",            65, 2);  // Por turno
        cola.encolar(siguienteId++, "Ana Mercedes Diaz Fernandez", 30, 2);  // Por turno
    }
};

// =================================================================
//  MENU PRINCIPAL
// =================================================================
void mostrarMenu(int enEspera)
{
    cout << "\n" << DLINEA << endl;
    cout << "     CLINICA DE DON FABIO - GESTION DE TURNOS" << endl;
    cout << DLINEA << endl;
    cout << "   Pacientes en espera: " << enEspera << endl;
    cout << LINEA << endl;
    cout << "   -- Gestion de turnos --" << endl;
    cout << "   [1]  Registrar paciente" << endl;
    cout << "   [2]  Ver sala de espera" << endl;
    cout << "   [3]  Atender siguiente paciente" << endl;
    cout << "\n   -- Cancelaciones --" << endl;
    cout << "   [4]  Cancelar una cita" << endl;
    cout << "   [5]  Reactivar una cita cancelada" << endl;
    cout << "   [6]  Ver citas canceladas" << endl;
    cout << "\n   -- Consultas y reportes --" << endl;
    cout << "   [7]  Buscar paciente" << endl;
    cout << "   [8]  Ver historial de atendidos" << endl;
    cout << "   [9]  Ordenar historial (edad / fecha)" << endl;
    cout << "   [10] Estadisticas" << endl;
    cout << "\n   [0]  Salir" << endl;
    cout << DLINEA << endl;
}

int main()
{
    Clinica clinica;
    clinica.cargarDatosPrueba();

    int opcion = -1;
    do
    {
        mostrarMenu(clinica.pacientesEnEspera());
        opcion = leerEntero(" Seleccione una opcion: ");

        // Si el flujo se cierra (EOF), salimos de forma ordenada.
        if (opcion == -1)
        {
            cout << "\n Cerrando el sistema. Hasta pronto." << endl;
            break;
        }

        switch (opcion)
        {
            case 1:  clinica.registrarPaciente();   break;
            case 2:  clinica.mostrarColaActual();    break;
            case 3:  clinica.atenderPaciente();      break;
            case 4:  clinica.cancelarCita();         break;
            case 5:  clinica.reactivarCita();        break;
            case 6:  clinica.mostrarCancelados();    break;
            case 7:  clinica.buscarPaciente();       break;
            case 8:  clinica.mostrarHistorial();     break;
            case 9:  clinica.ordenarHistorial();     break;
            case 10: clinica.mostrarEstadisticas();  break;
            case 0:  cout << "\n Gracias por usar el sistema. Hasta pronto." << endl; break;
            default: cout << "\n Opcion no valida. Intente de nuevo." << endl;
        }

    } while (opcion != 0);

    return 0;
}