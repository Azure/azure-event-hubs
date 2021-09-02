package com.microsoft.azure.eventhubs.samples.log4jeventhubappendersample;

import java.util.concurrent.TimeUnit;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/**
 * Hello world!
 *
 */
public class Log4JEventHubAppenderSample
{
    private static final Logger logger = LogManager.getLogger(Log4JEventHubAppenderSample.class);
    public static void main( String[] args )
    {
        int i;
                for(i=0; i<=10;i++)
                {
                        try {
                        TimeUnit.SECONDS.sleep(1);
                        logger.error("Hello World " + i);
                        }

                        catch(InterruptedException e)
                        {
                        }
                }
    }
}
