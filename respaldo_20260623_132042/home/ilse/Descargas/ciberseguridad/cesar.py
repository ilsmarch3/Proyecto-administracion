def descifrar_cesar(texto_cifrado):
    """
    Descifra un texto cifrado con César probando todos los desplazamientos (rot1 a rot25)
    
    Args:
        texto_cifrado (str): Texto cifrado a descifrar
    
    Returns:
        dict: Diccionario con desplazamiento y texto descifrado para cada rotación
    """
    resultados = {}
    
    # Probamos todos los desplazamientos posibles (1 al 25)
    for desplazamiento in range(1, 26):
        texto_descifrado = ""
        
        for caracter in texto_cifrado:
            # Verificamos si es una letra mayúscula
            if 'A' <= caracter <= 'Z':
                # Desplazamos hacia atrás (restamos) y aplicamos módulo 26
                nuevo_caracter = chr((ord(caracter) - ord('A') - desplazamiento) % 26 + ord('A'))
                texto_descifrado += nuevo_caracter
            
            # Verificamos si es una letra minúscula
            elif 'a' <= caracter <= 'z':
                nuevo_caracter = chr((ord(caracter) - ord('a') - desplazamiento) % 26 + ord('a'))
                texto_descifrado += nuevo_caracter
            
            # Si no es letra, lo dejamos igual
            else:
                texto_descifrado += caracter
        
        resultados[desplazamiento] = texto_descifrado
    
    return resultados


def mostrar_resultados(resultados):
    """Muestra los resultados de forma ordenada"""
    print("=" * 60)
    print("RESULTADOS DEL DESCIFRADO CÉSAR (TODAS LAS ROTACIONES)")
    print("=" * 60)
    
    for rotacion, texto in resultados.items():
        print(f"ROT{rotacion:2d}: {texto}")
    
    print("=" * 60)


# Ejemplo de uso
if __name__ == "__main__":
    # Texto de ejemplo
    texto_cifrado = input("Ingrese el texto cifrado: ")
    
    # Desciframos
    resultados = descifrar_cesar(texto_cifrado)
    
    # Mostramos resultados
    mostrar_resultados(resultados)
    
    # Opcional: guardar en archivo
    guardar = input("\n¿Desea guardar los resultados en un archivo? (s/n): ").lower()
    if guardar == 's':
        nombre_archivo = input("Nombre del archivo (ej: resultados.txt): ")
        with open(nombre_archivo, 'w', encoding='utf-8') as archivo:
            for rotacion, texto in resultados.items():
                archivo.write(f"ROT{rotacion:2d}: {texto}\n")
        print(f"Resultados guardados en {nombre_archivo}")
