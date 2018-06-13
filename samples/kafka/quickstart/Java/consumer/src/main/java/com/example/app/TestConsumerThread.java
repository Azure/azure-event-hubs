import org.apache.kafka.clients.consumer.*;
import org.apache.kafka.common.serialization.LongDeserializer;
import org.apache.kafka.common.serialization.StringDeserializer;
import java.io.FileReader;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Collections;
import java.util.Properties;

public class TestConsumerThread implements Runnable {

    private final String TOPIC;
    
    //Each consumer needs a unique client ID per thread
    private static int id = 0;

    public TestConsumerThread(final String TOPIC){
        this.TOPIC = TOPIC;
    }

    public void run (){
        final Consumer<Long, String> consumer = createConsumer();
        System.out.println("Polling");

        try {
            while (true) {
                final ConsumerRecords<Long, String> consumerRecords = consumer.poll(1000);
                for(ConsumerRecord<Long, String> cr : consumerRecords) {
                    System.out.printf("Consumer Record:(%d, %s, %d, %d)\n", cr.key(), cr.value(), cr.partition(), cr.offset());
                }
                consumer.commitAsync();
            }
        } catch (CommitFailedException e) {
            System.out.println("CommitFailedException: " + e);
        } finally {
            consumer.close();
        }
    }

    private Consumer<Long, String> createConsumer() {
        try {
            final Properties properties = new Properties();
            synchronized (TestConsumerThread.class) {
                properties.put(ConsumerConfig.CLIENT_ID_CONFIG, "KafkaExampleConsumer#" + id);
                id++;
            }
            properties.put(ConsumerConfig.KEY_DESERIALIZER_CLASS_CONFIG, LongDeserializer.class.getName());
            properties.put(ConsumerConfig.VALUE_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class.getName());

            //Get remaining properties from config file
            properties.load(new FileReader("src/main/resources/consumer.config"));

            // Create the consumer using properties.
            final Consumer<Long, String> consumer = new KafkaConsumer<>(properties);

            // Subscribe to the topic.
            consumer.subscribe(Collections.singletonList(TOPIC));
            return consumer;
            
        } catch (FileNotFoundException e){
            System.out.println("FileNoteFoundException: " + e);
            System.exit(1);
            return null;        //unreachable
        } catch (IOException e){
            System.out.println("IOException: " + e);
            System.exit(1);
            return null;        //unreachable
        }
    }
}
