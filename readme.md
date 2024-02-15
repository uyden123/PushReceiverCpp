# FCMReceiverCpp

C++ implementation of [push-receiver](https://github.com/MatthieuLemoine/push-receiver)

FCMReceiverCpp is a Firebase Cloud Messaging (FCM) push notification receiver written in C++. It allows you to register with the FCM server and listen for incoming push notifications.

## Usage

The program can be run with various command-line options:

- `-r` or `--register`: Register to the FCM server.
- `--register_input`: If set, the register info will be taken from this path. Otherwise, the system will attempt to find 'init_fcm_data.json' in the same directory as this executable being called.
- `--register_output`: If set, the register info file will be placed in this path. Otherwise, it will be placed in the same directory as this executable being called.
- `-l` or `--listen`: Listen to the FCM server.
- `--listen_input`: If set, the register info will be taken from this path. Otherwise, the system will attempt to find 'fcm_register_data.json' in the same directory as this executable being called.
- `--log_folder`: If set, log file 'FCMReceiver.log' will be placed in this folder. Otherwise, it will be placed in the same folder as this executable being called.
- `-h` or `--help`: Prints out the help message.
- `-v` or `--version`: Prints out the version.

## Example Usage

### Registering to the FCM server

To register to the FCM server using a specific input file and output file, you can use the `--register`, `--register_input`, and `--register_output` options:

```bash
FCMReceiverCpp --register --register_input /path/to/input.json --register_output /path/to/output.json
```

If you don't specify the input and output files, the program will use the default files (`init_fcm_data.json` and `fcm_register_data.json` respectively) in the same directory as the executable being called:

```bash
FCMReceiverCpp --register
```

### Listening to the FCM server

To listen to the FCM server using a specific input file, you can use the `--listen` and `--listen_input` options:

```bash
FCMReceiverCpp --listen --listen_input /path/to/input.json
```

If you don't specify the input file, the program will use the default file (`fcm_register_data.json`) in the same directory as the executable being called:

```bash
FCMReceiverCpp --listen
```

### Specifying the log folder

To specify the folder where the log file (`FCMReceiver.log`) will be placed, you can use the `--log_folder` option:

```bash
FCMReceiverCpp --register --log_folder /path/to/log/folder
```

### Displaying help and version information

To display the help message, you can use the `-h` or `--help` option:

```bash
FCMReceiverCpp --help
```

To display the version information, you can use the `-v` or `--version` option:

```bash
FCMReceiverCpp --version
```

## Example for `init_fcm_data.json`

If you wanna use --register argument then you must have `init_fcm_data.json` format as below:
Here is an example of how you might use the `init_fcm_data.json` file in the context of the FCMReceiverCpp program:

```json
{
  "appid": "1:56908307633:web:fe27804e72dda792b31690",
  "projectid": "test-413e0",
  "apikey": "AIzaSyAXAW5aPD3WqDc3YBCFbF9FezmUtlLv9DM",
  "vapidkey": "BMF_LgYJdGVsO-8hfbWgMFxrK7-R_UTnJBnkls5m3ZWPQc-NPTtaXOJLvqn8Rv_uBI6IXJV2ZKBoRxCIVflMKVY"
}
```

This JSON file contains the necessary credentials for the FCMReceiverCpp program to interact with the Firebase Cloud Messaging (FCM) server. 

- `appid`: This is the unique identifier of your Firebase app, which is used to connect to the FCM server.
- `projectid`: This is the identifier of your Firebase project. It's used to specify which Firebase project the app belongs to.
- `apikey`: This is the API key for your Firebase project. It's used to authenticate requests from your app to the Firebase services.
- `vapidkey`: This is the Voluntary Application Server Identification (VAPID) key. It's used for web push protocol in communication between the push service and the application server.

You would typically generate these values when you set up your Firebase project and app. They should be kept secure and not shared publicly. The `init_fcm_data.json` file should be located in a secure location and read by the FCMReceiverCpp program to establish a connection with the FCM server.

## Exit Codes

The program uses the following exit codes:

- `0`: Success
- `1`: Parse error
- `2`: Argument error
- `3`: Can't read register input file
- `4`: Register input file type invalid
- `5`: Register input data invalid
- `6`: Can't generate keys
- `7`: Register output file type invalid
- `8`: Register failed
- `9`: Can't write register data
- `10`: Listen input file type invalid
- `11`: Can't read listen input file
- `12`: Listen input data invalid
- `13`: Can't connect to FCM server
- `14`: Error while listening

For example, if the program exits with code 3, it means that it couldn't read the register input file, maybe it doesn't exists.