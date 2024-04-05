import javax.imageio.*;
import javax.swing.*;
import java.awt.*;
import java.io.*;
import java.net.Socket;
import java.net.URL;
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

    private Image texture_left, texture_right;

    private static Socket socket;

    private static ObjectOutputStream out;
    private static BufferedReader in;
    public ParticleSystemApp() {
        // Window Initialization
        setTitle("Particle Simulation Client");
        setSize(1280, 720); // The window itself
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLayout(new BorderLayout());
        setResizable(false);
        getContentPane().setBackground(Color.GRAY); // Set the default window color

        particleBatchMap = new HashMap<>();
        Buddies = new ArrayList<>();

        //Initializing the batch list
        particleBatchList = new ArrayList<ParticleBatch>();
        ParticleBatch tempPb = new ParticleBatch(0);
        tempPb.start();
        particleBatchList.add(tempPb);

        try {
            URL imageUrl_left = getClass().getResource("/ghost_left.png");
            texture_left = ImageIO.read(imageUrl_left);

            URL imageUrl_right = getClass().getResource("/ghost_right.png");
            texture_right = ImageIO.read(imageUrl_right);
        } catch (IOException e) {
            e.printStackTrace();
        }

        Thread t1 = new Thread(new Runnable() {
            @Override
            public void run() {
                receiveThread();
            }
        });
        t1.start();

        character = new Ghost(1,1,texture_left, texture_right);
        Thread t2 = new Thread(new Runnable() {
            @Override
            public void run() {
                sendThread();
            }
        });
        t2.start();

        // Particle System Panel
        particlePanel = new ParticlePanel(particleBatchList, character, Buddies);
        add(particlePanel, BorderLayout.CENTER);

        pack(); // Adjusts the frame size to fit the preferred size of its components
        setLocationRelativeTo(null); // Centers the frame on the screen

        particlePanel.requestFocus();
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

            // SERVER_IP = input
            String address = "localhost"; // TEMPORARY
            //InetAddress address = InetAddress.getByName(SERVER_IP); // Enter host ip address
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
                // Receive JSON data from the server
                InputStream inputStream = socket.getInputStream();
                BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
                String serializedCoordinates = reader.readLine(); // Assuming each message is a single line

                JSONArray jsonArray = new JSONArray(serializedCoordinates);
                // Get the type of message
                int type = 0;

                // Decode the received json
                for (int i = 0; i < jsonArray.length(); i++) {
                    JSONObject jsonObject = jsonArray.getJSONObject(i);

                    if (i == 0) {
                        type = jsonObject.getInt("Type");
                    }
                    else {
                        switch(type) {
                            case 1: // Received buddy client coordinates
                                int clientX = jsonObject.getInt("X");
                                int clientY = jsonObject.getInt("Y");
                                String id = jsonObject.getString("ID");

                                for (Ghost buddy: Buddies) {
                                    if (buddy.getId().equals(id))
                                        buddy.updatePos(clientX, clientY);
                                }
                                break;

                            case 2: // Received particle coordinates
                                int batchId = jsonObject.getInt("BatchID");
                                int x = jsonObject.getInt("X");
                                int y = jsonObject.getInt("Y");
                                int theta = jsonObject.getInt("Theta");
                                int velocity = jsonObject.getInt("Velocity");

                                particleBatchMap.computeIfAbsent(batchId, k -> new ParticleBatch(batchId)).addParticle(new Particle(x, y, (double) theta, (double) velocity));
                                break;

                            case 3: // Buddy got deleted :(
                                String delete_id = jsonObject.getString("ID");
                                Buddies.removeIf(obj -> obj.getId().equals(delete_id));
                                break;

                            default: // Error type
                                System.out.println("Error Type");
                                break;
                        }
                    }
                }

                // If end then break
                // running = false; // Uncomment to break the loop based on a condition
            } catch (IOException e) {
                e.printStackTrace(); // Or handle the exception as needed
                break;
                // Handle the exception as needed, possibly by breaking out of the loop
            } catch (JSONException e) {
                e.printStackTrace(); // Or handle the exception as needed
                // Handle the exception as needed
                break;
            }
        }

        System.out.println("Broke out of the loop");
    }

    public void sendThread() {
        while (true) {
            try {
                PrintWriter out = new PrintWriter(new BufferedWriter(new OutputStreamWriter(socket.getOutputStream())), true);

                JSONObject jsonObject = new JSONObject();
                jsonObject.put("ClientID", character.getId());
                jsonObject.put("X", character.getX());
                jsonObject.put("Y", character.getY());

                String jsonString = jsonObject.toString();

                out.println(jsonString);
                // running = false; // Uncomment to break the loop based on a condition
            } catch (IOException e) {
                e.printStackTrace(); // Or handle the exception as needed
                break;
                // Handle the exception as needed, possibly by breaking out of the loop
            } catch (JSONException e) {
                e.printStackTrace(); // Or handle the exception as needed
                // Handle the exception as needed
                break;
            }
        }

        System.out.println("Broke out of the loop");
    }



    public class ServerAddressDialog {
        public static String showDialog() {
            String serverAddress = JOptionPane.showInputDialog(null, "Enter server address:", "Server Connection", JOptionPane.QUESTION_MESSAGE);
            return serverAddress;
        }
    }
}
