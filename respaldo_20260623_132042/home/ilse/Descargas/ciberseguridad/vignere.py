#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Cifrado de Vigenère
Permite encriptar y desencriptar textos usando una clave.
Soporta mayúsculas, minúsculas y mantiene caracteres no alfabéticos.
"""

import sys

def vigenere_cipher(text, key, mode='encrypt'):
    """
    Implementa el cifrado de Vigenère.
    
    Args:
        text (str): Texto a procesar
        key (str): Clave para el cifrado
        mode (str): 'encrypt' para encriptar, 'decrypt' para desencriptar
    
    Returns:
        str: Texto procesado
    """
    result = []
    key = key.upper()
    key_length = len(key)
    key_index = 0
    
    for char in text:
        if char.isalpha():
            # Determinar el desplazamiento según la clave
            shift = ord(key[key_index % key_length]) - ord('A')
            
            if mode == 'decrypt':
                shift = -shift
            
            # Procesar mayúsculas y minúsculas
            if char.isupper():
                shifted = (ord(char) - ord('A') + shift) % 26
                result.append(chr(shifted + ord('A')))
            else:  # minúscula
                shifted = (ord(char) - ord('a') + shift) % 26
                result.append(chr(shifted + ord('a')))
            
            key_index += 1
        else:
            # Mantener caracteres no alfabéticos sin cambios
            result.append(char)
    
    return ''.join(result)

def encrypt(text, key):
    """Encripta un texto usando el cifrado de Vigenère."""
    return vigenere_cipher(text, key, 'encrypt')

def decrypt(text, key):
    """Desencripta un texto usando el cifrado de Vigenère."""
    return vigenere_cipher(text, key, 'decrypt')

def main():
    """Función principal con interfaz de usuario."""
    print("=" * 50)
    print("CIFRADO DE VIGENÈRE")
    print("=" * 50)
    
    while True:
        print("\nOpciones:")
        print("1. Encriptar texto")
        print("2. Desencriptar texto")
        print("3. Salir")
        
        opcion = input("\nSeleccione una opción (1-3): ").strip()
        
        if opcion == '1':
            texto = input("Ingrese el texto a encriptar: ")
            clave = input("Ingrese la clave: ")
            
            if not clave:
                print("Error: La clave no puede estar vacía.")
                continue
            
            resultado = encrypt(texto, clave)
            print("\n" + "=" * 50)
            print(f"Texto original: {texto}")
            print(f"Clave: {clave}")
            print(f"Texto encriptado: {resultado}")
            print("=" * 50)
        
        elif opcion == '2':
            texto = input("Ingrese el texto a desencriptar: ")
            clave = input("Ingrese la clave: ")
            
            if not clave:
                print("Error: La clave no puede estar vacía.")
                continue
            
            resultado = decrypt(texto, clave)
            print("\n" + "=" * 50)
            print(f"Texto encriptado: {texto}")
            print(f"Clave: {clave}")
            print(f"Texto desencriptado: {resultado}")
            print("=" * 50)
        
        elif opcion == '3':
            print("\n¡Hasta luego!")
            sys.exit(0)
        
        else:
            print("Opción no válida. Intente de nuevo.")

if __name__ == "__main__":
    # Ejemplo de uso directo
    if len(sys.argv) > 1:
        # Uso desde línea de comandos
        if sys.argv[1] == '-e' and len(sys.argv) == 4:
            resultado = encrypt(sys.argv[2], sys.argv[3])
            print(f"Encriptado: {resultado}")
        elif sys.argv[1] == '-d' and len(sys.argv) == 4:
            resultado = decrypt(sys.argv[2], sys.argv[3])
            print(f"Desencriptado: {resultado}")
        else:
            print("Uso:")
            print("  python vigenere.py -e \"texto\" \"clave\"  # Encriptar")
            print("  python vigenere.py -d \"texto\" \"clave\"  # Desencriptar")
            print("\nO ejecutar sin argumentos para modo interactivo.")
    else:
        main()
