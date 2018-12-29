# otp
A simple one time pad encryption implementation.

Utilizes a client/server model to encrypt and decrypt plaintext files.

### keygen.c
This file creates a key used for file encryption and decryption. It must be given a desired key length.

### otp_enc_d.c
This is the encryption daemon. It encrypts the plaintext file by listening on a socket for a file to encrypt and the corresponding key. Once a request is received, a child process is created to receive the file and key, commence encryption, and send back the encrypted file contents. 

### otp_enc.c
This is the encryption client. It connects to the encryption daemon and requests an OTP style encryption. Once the request is processed, it receives and displays the response to standard output.

### otp_dec_d.c
This is the decryption daemon. It works similar to the encryption server described above.

### otp_dec.c
This is the decryption client. It works similar to the encryption client described above.
----------------------------------------------------------------------------------------------------------------------------------------

### Example:

	$ ./otp_enc_d 8888 &	
	$ ./otp_dec_d 8889 &	
	$ ./keygen 16 > key	
	$ cat text_to_encrypt
	$ IN THE BEGINNING
	$ ./otp_enc text_to_encrypt key 8888 > ciphertext
	$ cat ciphertext	
	$ YAQZYRJZWKMDVQTW	
	$ ./otp_dec ciphertext key 8889 > text_to_encrypt_1	
	$ cat text_tp_encrypt_1
	$ IN THE BEGINNING