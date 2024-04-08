import javax.imageio.*;
import javax.swing.*;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.*;
import java.net.*;
import java.util.*;
import java.util.List;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;


// CLIENT FOR PARTICLE SIMULATOR
public class ParticleSystemApp extends JFrame {

    private final Object particleListLock = new Object();

    // GUI attributes
    private ParticlePanel particlePanel;

    private ArrayList<Ghost> Buddies;
    private Ghost character;

    private HashMap<Integer, ParticleBatch> particleBatchMap;
    private ArrayList<ParticleBatch> particleBatchList;

    private Image texture_left;
    private Image texture_right;

    private static Socket socket;
    private boolean isSocketClosed = false;

    private ObjectOutputStream out;
    private BufferedReader in;
    public ParticleSystemApp() {
        // Window Initialization
        setTitle("Particle Simulation Client");
        setSize(1280, 720); // The window itself
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLayout(new BorderLayout());
        setResizable(false);
        getContentPane().setBackground(Color.GRAY); // Set the default window color

        try {
            URL imageUrl_left = getClass().getResource("/ghost_left.png");
            texture_left = ImageIO.read(imageUrl_left);

            URL imageUrl_right = getClass().getResource("/ghost_right.png");
            texture_right = ImageIO.read(imageUrl_right);
        } catch (IOException e) {
            e.printStackTrace();
        }

        character = new Ghost(1,1, texture_left, texture_right);

        particleBatchMap = new HashMap<>();
        Buddies = new ArrayList<>();

        //Initializing the batch list
        particleBatchList = new ArrayList<ParticleBatch>();

        Thread t1 = new Thread(new Runnable() {
            @Override
            public void run() {
                receiveThread();
            }
        });
        t1.start();


        // Particle System Panel
        particlePanel = new ParticlePanel(particleBatchList, character, Buddies);
        add(particlePanel, BorderLayout.CENTER);

        pack(); // Adjusts the frame size to fit the preferred size of its components
        setLocationRelativeTo(null); // Centers the frame on the screen

        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                System.out.println("Closing connection to the server...");
                closeSocket(); // Use the method to close the socket
                System.out.println("Connection to the server closed.");
                System.exit(0); // Exit the application
            }
        });


        addKeyListener(new KeyAdapter() {
            @Override
            public void keyPressed(KeyEvent e) {
                int dx = 0, dy = 0;

                switch (e.getKeyCode()) {
                    case KeyEvent.VK_W:
                    case KeyEvent.VK_UP:
                        dy = 1;
                        break;
                    case KeyEvent.VK_S:
                    case KeyEvent.VK_DOWN:
                        dy = -1;
                        break;
                    case KeyEvent.VK_A:
                    case KeyEvent.VK_LEFT:
                        dx = -1;
                        character.turnChar(true);
                        break;
                    case KeyEvent.VK_D:
                    case KeyEvent.VK_RIGHT:
                        dx = 1;
                        character.turnChar(false);
                        break;
                }
                // Update the character's position locally
                character.move(dx, dy);
                particlePanel.repaint(); // Redraw the panel to reflect the character's new position

                // Send the updated position to the server
                sendThread();
            }
        });


        // Start gamelogic thread
        new Thread(this::gameLoop).start();
    }

    private void gameLoop() {
        final double maxFramesPerSecond = 60.0;
        final long frameTime = (long) (1000 / maxFramesPerSecond);
        long lastFrameTime = System.currentTimeMillis();

        while (true) {
            long currentFrameTime = System.currentTimeMillis();
            long deltaTime = currentFrameTime - lastFrameTime; // Calculate the elapsed time since the last frame
            lastFrameTime = currentFrameTime;

            // Update each particle's position based on deltaTime
            synchronized (particleListLock) {
                for (ParticleBatch batch : particleBatchList) {
                    for (Particle particle : batch.getParticles()) { // Assuming getParticles() gives access to the particles in the batch
                        particle.update(deltaTime);
                    }
                }
            }


            SwingUtilities.invokeLater(particlePanel::repaint);

            long endTime = System.currentTimeMillis();
            long frameDuration = endTime - currentFrameTime;
            long sleepTime = frameTime - frameDuration;

            if (sleepTime > 0) {
                try {
                    Thread.sleep(sleepTime);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }
        }
    }

    public static void main(String[] args) {
        String serverAddress = "";
        boolean connected = false;

            while (!connected) {
                serverAddress = ServerAddressDialog.showDialog();
                if (serverAddress != null && !serverAddress.isEmpty()) {
                    connected = clientConnect(serverAddress);
                    if (!connected) {
                        JOptionPane.showMessageDialog(null, "Failed to connect to the server. Please try again.", "Connection Failed", JOptionPane.ERROR_MESSAGE);
                    }
                } else {
                    // User closed the dialog without entering an address
                    System.exit(0); // Exit the application
                }
            }

        // Proceed with the application after a successful connection
        System.out.println("Successfully connected to the server: " + serverAddress);

        SwingUtilities.invokeLater(() -> {
            ParticleSystemApp app = new ParticleSystemApp();
            app.setVisible(true);
        });
    }

    // Connect to server
    public static Boolean clientConnect(String input) {
        try {
            // Socket

            String SERVER_IP = input;
            // String address = "localhost"; // TEMPORARY
            InetAddress address = InetAddress.getByName(SERVER_IP); // Enter host ip address
            int CLIENT_PORT = 12345;

            socket = new Socket(address, CLIENT_PORT);

            return true;
        }
        catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    // Receiving thread
    public void receiveThread() {
        while (true) {
            try {
                // Check if the socket is closed before attempting to read from it
                if (socket.isClosed()) {
                    System.out.println("Socket is closed. Exiting receiveThread.");
                    break; // Exit the loop if the socket is closed
                }
                
                // Receive JSON data from the server
                InputStream inputStream = socket.getInputStream();
                BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
                String serializedCoordinates = reader.readLine(); // Assuming each message is a single line
                System.out.println("\nReceived data: " + serializedCoordinates + "\n");

                // Check if the end of the stream has been reached
                if (serializedCoordinates == null) {
                    System.out.println("Server has closed the connection. Exiting receiveThread.");
                    break; // Exit the loop if the server has closed the connection
                }

                JSONArray jsonArray = new JSONArray(serializedCoordinates);
                // Get the type of message
                int type = -1;

                // Decode the received json
                for (int i = 0; i < jsonArray.length(); i++) {
                    JSONObject jsonObject = jsonArray.getJSONObject(i);
                    System.out.println(jsonObject.toString());

                    if (i == 0) {
                        type = jsonObject.getInt("Type");
                    }
                    else {
                        switch(type) {
                            case 0: // Welcome
                                int self_id = jsonObject.getInt("ID");
                                character.setID(self_id);
                                System.out.println(character.getId());

                                break;
                            case 1: // Received buddy client coordinates
                                int clientX = jsonObject.getInt("X");
                                int clientY = jsonObject.getInt("Y");
                                int id = jsonObject.getInt("ClientID");

                                // Print the buddy's client ID and details
                                System.out.println("Buddy Client ID: " + id + ", X: " + clientX + ", Y: " + clientY);

                                for (Ghost buddy: Buddies) {
                                    if (buddy.getId() == id) {
                                        buddy.updatePos(clientX, clientY);
                                    }
                                }
                                break;

                            case 2: // Received particle coordinates
                                int batchId = jsonObject.getInt("BatchID");
                                int x = jsonObject.getInt("X");
                                int y = jsonObject.getInt("Y");
                                int theta = jsonObject.getInt("Theta");
                                int velocity = jsonObject.getInt("Velocity");

                                if (particleBatchMap.containsKey(batchId)) {
                                    ParticleBatch batch = particleBatchMap.get(batchId);
                                    batch.addParticle(new Particle(x, y, (double) theta, (double) velocity));
                                }
                                else {
                                    ParticleBatch newBatch = new ParticleBatch(batchId);
                                    newBatch.addParticle(new Particle(x, y, (double) theta, (double) velocity));
                                    particleBatchMap.put(batchId, newBatch);
                                    particleBatchList.add(newBatch);
                                }
                                break;

                            case 3: // Buddy got deleted :(
                                int delete_id = jsonObject.getInt("ClientID");
                                Buddies.removeIf(obj -> obj.is(delete_id));
                                break;

                            default: // Error type
                                System.out.println("Error Type");
                                break;
                        }
                    }
                }

                if (type == 2) {
                    System.out.println("List of batches");
                    for (ParticleBatch batch: particleBatchList) {
                        System.out.format("%d has %d particles\n", batch.getId(), batch.getNumParticles());
                    }
                }

                // If end then break
                // running = false; // Uncomment to break the loop based on a condition
            } catch (SocketException e) {
                if (e.getMessage().contains("Connection reset")) {
                    System.out.println("Server has abruptly closed the connection. Exiting receiveThread.");
                } else {
                    e.printStackTrace(); // Handle other SocketExceptions as needed
                }
                break;
            } catch (IOException e) {
                e.printStackTrace(); // Handle other IOExceptions as needed
                break;
            } catch (JSONException e) {
                e.printStackTrace(); // Or handle the exception as needed
                // Handle the exception as needed
                break;
            }
        }

        // Close resources properly
        try {
            if (in != null) in.close();
            if (out != null) out.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        System.out.println("Broke out of the loop");

        // Close the client window and exit the application
        System.out.println("Server has closed the connection. Closing the client window.");
        SwingUtilities.invokeLater(() -> {
            ParticleSystemApp.this.dispose(); // Close the JFrame
            System.exit(0); // Exit the application
        });
    }

    public void sendThread() {
        try {
            if (socket.isClosed()) {
                System.out.println("Socket is closed. Cannot send data.");
                return;
            }

            // Create a JSON object for the current position
            JSONObject positionObject = new JSONObject();
            positionObject.put("ClientID", character.getId());
            positionObject.put("X", character.getX());
            positionObject.put("Y", character.getY());

            // Convert the JSON object to a string
            String message = positionObject.toString();

            // Send the message to the server
            PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
            out.println(message);
        } catch (IOException e) {
            e.printStackTrace();
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }




    public void closeSocket() {
        if (!isSocketClosed) {
            try {
                if (socket != null && !socket.isClosed()) {
                    socket.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                isSocketClosed = true;
            }
        }
    }



    public class ServerAddressDialog {
        public static String showDialog() {
            String serverAddress = JOptionPane.showInputDialog(null, "Enter server address:", "Server Connection", JOptionPane.QUESTION_MESSAGE);
            return serverAddress;
        }
    }
}
